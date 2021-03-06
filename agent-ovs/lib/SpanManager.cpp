/* -*- C++ -*-; c-basic-offset: 4; indent-tabs-mode: nil */
/*
 * Implementation for SpanManager class.
 *
 * Copyright (c) 2019 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <opflexagent/SpanManager.h>
#include <opflexagent/logging.h>
#include <modelgbp/span/Universe.hpp>
#include <modelgbp/epr/L2Universe.hpp>
#include <modelgbp/epdr/EndPointToGroupRSrc.hpp>

namespace opflexagent {

    using namespace std;
    using namespace modelgbp::epr;
    using namespace modelgbp::epdr;
    using namespace modelgbp::gbp;
    using namespace opflex::modb;
    using opflex::modb::class_id_t;
    using opflex::modb::URI;

    recursive_mutex SpanManager::updates;

    SpanManager::SpanManager(opflex::ofcore::OFFramework &framework_,
                             boost::asio::io_service& agent_io_) :
            spanUniverseListener(*this), framework(framework_),
            taskQueue(agent_io_){}

    void SpanManager::start() {
        LOG(DEBUG) << "starting span manager";
        Universe::registerListener(framework, &spanUniverseListener);
        Session::registerListener(framework,&spanUniverseListener);
        LocalEp::registerListener(framework, &spanUniverseListener);
        L2Ep::registerListener(framework, &spanUniverseListener);
    }

    void SpanManager::stop() {
        Universe::unregisterListener(framework, &spanUniverseListener);
        LocalEp::unregisterListener(framework, &spanUniverseListener);
        L2Ep::unregisterListener(framework, &spanUniverseListener);
    }

    SpanManager::SpanUniverseListener::SpanUniverseListener(SpanManager &spanManager) :
            spanmanager(spanManager) {}

    SpanManager::SpanUniverseListener::~SpanUniverseListener() {}

    void SpanManager::SpanUniverseListener::objectUpdated(class_id_t classId,
                                                          const URI &uri) {
        lock_guard <recursive_mutex> guard(SpanManager::updates);

        // updates on parent container for session are received for
        // session creation. Deletion and modification updates are
        // sent to the object itself.
        if (classId == Universe::CLASS_ID) {
            optional<shared_ptr<Universe>> univ_opt =
                Universe::resolve(spanmanager.framework);
            if (univ_opt) {
                vector <shared_ptr<Session>> sessVec;
                univ_opt.get()->resolveSpanSession(sessVec);
                for (const shared_ptr<Session>& sess : sessVec) {
                    auto itr = spanmanager.sess_map.find(sess->getURI());
                    if (itr == spanmanager.sess_map.end()) {
                        LOG(DEBUG) << "creating session " << sess->getURI();
                        spanmanager.processSession(sess);
                    }
                    spanmanager.notifyUpdate.insert(sess->getURI());
                 }
            }
        } else if (classId == LocalEp::CLASS_ID) {
            optional<shared_ptr<LocalEp>> lEp =
                    LocalEp::resolve(spanmanager.framework, uri);
            if (lEp) {
                optional<URI> sesUri = SpanManager::getSession(lEp.get());
                if (sesUri) {
                    optional<shared_ptr<SrcMember>> pSmem =
                        spanmanager.findSrcMem(sesUri.get(), lEp.get()->getURI());
                    if (pSmem) {
                        optional<const unsigned char> dir = pSmem.get()->getDir();
                        if (dir) {
                            spanmanager.processLocalEp(uri, dir.get());
                        }
                    }
                }
            }
        } else if (classId == L2Ep::CLASS_ID) {
            auto l2Ep = L2Ep::resolve(spanmanager.framework, uri);
            if (l2Ep) {
                spanmanager.processL2Ep(l2Ep.get());
            }
        } else if (classId == Session::CLASS_ID) {
            optional<shared_ptr<Session>> sess =
                Session::resolve(spanmanager.framework, uri);
            if (sess) {
                LOG(DEBUG) << "update on session " << sess.get()->getURI();
                spanmanager.processSession(sess.get());
                spanmanager.notifyUpdate.insert(uri);
            } else {
                LOG(DEBUG) << "session removed " << uri;
                shared_ptr<SessionState> sessState;
                auto itr = spanmanager.sess_map.find(uri);
                if (itr != spanmanager.sess_map.end()) {
                    shared_ptr<SessionState> state = itr->second;
                    spanmanager.notifyDelete.insert(state);
                    spanmanager.sess_map.erase(itr);
                }
            }
        }
        // notify all listeners. put it on a task Q for non blocking notification.
        for (const URI& updateUri : spanmanager.notifyUpdate) {
            spanmanager.taskQueue.dispatch(updateUri.toString(), [=]() {
                spanmanager.notifyListeners(updateUri);
            });
        }
        spanmanager.notifyUpdate.clear();
        for (const shared_ptr<SessionState>& session : spanmanager.notifyDelete) {
            spanmanager.taskQueue.dispatch(session->getName(), [=]() {
                spanmanager.notifyListeners(session);
            });
        }
        spanmanager.notifyDelete.clear();
    }

    void SpanManager::registerListener(SpanListener *listener) {
        lock_guard<mutex> guard(listener_mutex);
        LOG(DEBUG) << "registering listener";
        spanListeners.push_back(listener);
    }

    void SpanManager::unregisterListener(SpanListener* listener) {
        lock_guard<mutex> guard(listener_mutex);
        spanListeners.remove(listener);
    }

    void SpanManager::notifyListeners(const URI& spanURI) {
        lock_guard<mutex> guard(listener_mutex);
        for (SpanListener *listener : spanListeners) {
            listener->spanUpdated(spanURI);
        }
    }

    void SpanManager::notifyListeners(const shared_ptr<SessionState>& seSt) {
        lock_guard<mutex> guard(listener_mutex);
        for (SpanListener *listener : spanListeners) {
            listener->spanDeleted(seSt);
        }
    }

    optional<shared_ptr<SessionState>> SpanManager::getSessionState(const URI& uri) {
        lock_guard <recursive_mutex> guard(updates);
        auto itr = sess_map.find(uri);
        if (itr == sess_map.end()) {
            return boost::none;
        } else {
            return itr->second;
        }
    }

    void SpanManager::processSession(const shared_ptr<Session>& sess) {
        shared_ptr<SessionState> sessState =
            make_shared<SessionState>(sess->getURI(), sess->getName().get());
        sess_map[sess->getURI()] = sessState;
        sessState->setAdminState(sess->getState(1));

        vector <shared_ptr<SrcGrp>> srcGrpVec;
        sess->resolveSpanSrcGrp(srcGrpVec);
        for (const shared_ptr<SrcGrp>& srcGrp : srcGrpVec) {
            processSrcGrp(srcGrp);
        }
        vector <shared_ptr<DstGrp>> dstGrpVec;
        sess->resolveSpanDstGrp(dstGrpVec);
        for (const shared_ptr<DstGrp>& dstGrp : dstGrpVec) {
            processDstGrp(dstGrp, sess->getURI());
        }
    }

    void SpanManager::processSrcGrp(const shared_ptr<SrcGrp>& srcGrp) {
        vector<shared_ptr<SrcMember>> srcMemVec;
        srcGrp->resolveSpanSrcMember(srcMemVec);
        for (const shared_ptr<SrcMember>& srcMem : srcMemVec) {
            optional <shared_ptr<MemberToRefRSrc>> memRefOpt =
                srcMem->resolveSpanMemberToRefRSrc();
            if (memRefOpt) {
                shared_ptr <MemberToRefRSrc> memRef = memRefOpt.get();
                if (memRef->getTargetClass()) {
                    class_id_t class_id = memRef->getTargetClass().get();
                    if (class_id == modelgbp::gbp::EpGroup::CLASS_ID) {
                        if (memRef->getTargetURI()) {
                            URI pUri = memRef->getTargetURI().get();
                            LOG(DEBUG) << pUri.toString();
                            optional<const unsigned char> dir = srcMem->getDir();
                            if (dir) {
                                processEpGroup(pUri, dir.get());
                            }
                        }
                    } else if (class_id == LocalEp::CLASS_ID) {
                        if (memRef->getTargetURI()) {
                            URI pUri = memRef->getTargetURI().get();
                            optional<const unsigned char> dir = srcMem->getDir();
                            if (dir) {
                                processLocalEp(pUri, dir.get());
                            }
                        }
                    }
                }
            }
        }
    }

    void SpanManager::processDstGrp(const shared_ptr<DstGrp>& dstGrp, const URI& sessUri) {
        unordered_map<URI, shared_ptr<SessionState>>::const_iterator seSt = sess_map.find(sessUri);
        if (seSt != sess_map.end()) {
            vector <shared_ptr<DstMember>> dstMemVec;
            dstGrp->resolveSpanDstMember(dstMemVec);
            for (shared_ptr<DstMember>& dstMem : dstMemVec) {
                optional <shared_ptr<DstSummary>> dstSumm = dstMem->resolveSpanDstSummary();
                if (dstSumm) {
                    optional<const string &> dest = dstSumm.get()->getDest();
                    if (dest) {
                        address ip = boost::asio::ip::address::from_string(dest.get());
                        seSt->second->setDestination(ip);
                        if (dstSumm.get()->getVersion()) {
                            seSt->second->setVersion(dstSumm.get()->getVersion().get());
                        }
                    }
                }
            }
        }
    }

    void SessionState::addSrcEndpoint(const SourceEndpoint& srcEp) {
        LOG(DEBUG) << "Adding src end point " << srcEp.getName();
        lock_guard<recursive_mutex> guard(opflexagent::SpanManager::updates);
        srcEndpoints.emplace(srcEp);
    }

    void SpanManager::processLocalEp(const URI& uri, unsigned char dir) {
        if (LocalEp::resolve(framework, uri)) {
            shared_ptr<LocalEp> lEp = LocalEp::resolve(framework, uri).get();
            auto epRSrcOpt = lEp->resolveSpanLocalEpToEpRSrc();
            if (epRSrcOpt) {
                shared_ptr<LocalEpToEpRSrc> epRSrc = epRSrcOpt.get();
                auto epUriOpt = epRSrc->getTargetURI();
                if (epUriOpt) {
                    const URI& epUri = epUriOpt.get();
                    if (L2Ep::resolve(framework, epUri)) {
                        shared_ptr <L2Ep> l2Ep = L2Ep::resolve(framework, epUri).get();
                        addEndpoint(lEp, l2Ep, dir);
                    } else {
                        l2EpUri.emplace(epUri, lEp);
                    }
                }
            }
        }
    }

    void SpanManager::processEpGroup(const URI& uri, unsigned char dir) {
        LOG(DEBUG) << "Epg uri " << uri;
        // get the local end points that are part of this EP group
        std::vector<OF_SHARED_PTR<modelgbp::epr::L2Ep>> out;
        optional<shared_ptr<L2Universe>> l2u = L2Universe::resolve(framework);
        l2u.get()->resolveEprL2Ep(out);
        vector<shared_ptr<L2Ep>> l2EpVec;
        for (auto& ep : out) {
            LOG(DEBUG) << "ep " << ep->getURI();
            URI egUri(ep->getGroup().get());
            LOG(DEBUG) << "epg uri " << egUri;
            if (uri == egUri) {
                LOG(DEBUG) << "found L2Ep " << ep->getURI();
                l2EpVec.push_back(ep);
            }
        }
        optional<shared_ptr<modelgbp::gbp::EpGroup>> oEpGrp = EpGroup::resolve(framework, uri);
        if (!oEpGrp) {
            LOG(DEBUG) << "EpGroup " << uri << " not found";
            return;
        }
        if (l2EpVec.empty()) {
            LOG(DEBUG) << "No L2Eps found";
            return;
        }
        // get the span sessions associated with this EP group and
        // add L2Ep to the source end point list for each session.
        shared_ptr<modelgbp::gbp::EpGroup> lEpGrp = oEpGrp.get();
        std::vector<OF_SHARED_PTR<modelgbp::gbp::EpGroupToSpanSessionRSrc>> vGrpToSess;
        lEpGrp->resolveGbpEpGroupToSpanSessionRSrc(vGrpToSess);
        for (auto& sesRsrc : vGrpToSess) {
            unordered_map<opflex::modb::URI, shared_ptr<SessionState>>::iterator it;
            it = sess_map.find(sesRsrc->getTargetURI().get());
            if (it != sess_map.end()) {
                LOG(DEBUG) << "found session " << sesRsrc->getTargetURI().get();
                for (auto& ep : l2EpVec) {
                    SourceEndpoint srcEp(ep->getURI().toString(),
                                         ep->getInterfaceName().get(),
                                         dir);
                    it->second->addSrcEndpoint(srcEp);
                }
                notifyUpdate.insert(sesRsrc->getTargetURI().get());
            }
        }
    }

    bool SessionState::hasSrcEndpoints() const {
        lock_guard<recursive_mutex> guard(opflexagent::SpanManager::updates);
        return !srcEndpoints.empty();
    }

    void SessionState::getSrcEndpointSet(srcEpSet& ep) {
        lock_guard<recursive_mutex> guard(opflexagent::SpanManager::updates);
        ep.insert(srcEndpoints.begin(), srcEndpoints.end());
    }

    void SpanManager::addEndpoint(const shared_ptr<LocalEp>& lEp, const shared_ptr<L2Ep>& l2Ep, const unsigned char dir) {
        optional<URI> parent = SpanManager::getSession(lEp);
        if (parent) {
            notifyUpdate.insert(parent.get());
            auto sess = Session::resolve(framework, parent.get());
            if (sess) {
                auto itr = sess_map.find(sess.get()->getURI());
                if (itr != sess_map.end()) {
                    shared_ptr<SessionState> sesSt = sess_map[sess.get()->getURI()];
                    SourceEndpoint srcEp(lEp->getName().get(),
                                         l2Ep->getInterfaceName().get(),
                                         dir);
                    sesSt->addSrcEndpoint(srcEp);
                    notifyUpdate.insert(sess.get()->getURI());
                }
            }
        }
    }

    /**
     * Find the span session URI by walking back the elements of the LocalEp
     * URI. The span session URI will be the one prior to the element "SpanLocalEp".
     */
    const optional<URI> SpanManager::getSession(const shared_ptr<LocalEp>& lEp) {
        string uriStr;
        vector<string> elements;
        lEp->getURI().getElements(elements);
        auto rit = elements.rbegin();
        for (; rit != elements.rend(); ++rit) {
            if ((*rit) == "SpanLocalEp") {
                rit++;
                for (;rit != elements.rend(); ++rit) {
                    string temp("/");
                    temp.append(*rit);
                    uriStr.insert(0, temp);
                }
                uriStr.append("/");
                break;
            }
        }
        optional<URI> uri;
        if (!uriStr.empty()) {
            uri = URI(uriStr);
        }
        return uri;
    }

    optional<shared_ptr<SrcMember>> SpanManager::findSrcMem(const URI& sessUri, const URI& uri) {
        optional<shared_ptr<SrcMember>> pSrcMem;
        optional<shared_ptr<Session>> sess = Session::resolve(framework,
                        sessUri);
        if (sess) {
            vector <shared_ptr<SrcGrp>> srcGrpVec;
            sess.get()->resolveSpanSrcGrp(srcGrpVec);
            for (shared_ptr<SrcGrp>& srcGrp : srcGrpVec) {
                vector<shared_ptr<SrcMember>> srcMemVec;
                srcGrp->resolveSpanSrcMember(srcMemVec);
                for (shared_ptr<SrcMember>& srcMem : srcMemVec) {
                    optional <shared_ptr<MemberToRefRSrc>> memRefOpt =
                            srcMem->resolveSpanMemberToRefRSrc();
                    if (memRefOpt) {
                        shared_ptr <MemberToRefRSrc> memRef = memRefOpt.get();
                        if (memRef->getTargetURI()) {
                            URI tUri = memRef->getTargetURI().get();
                            if (uri == tUri) {
                                LOG(DEBUG) << "found src member for " << uri;
                                pSrcMem.reset(srcMem);
                                break;
                            }
                        }
                    }
                }
             }
        }
        return pSrcMem;
    }

    void SpanManager::processL2Ep(shared_ptr<L2Ep>& l2Ep) {
        auto itr = l2EpUri.find(l2Ep->getURI());
        if (itr != l2EpUri.end()) {
            optional<URI> sessUri = getSession(itr->second);
            if (sessUri && (sess_map.find(sessUri.get())
                    != sess_map.end())) {
                auto pSmem = findSrcMem(sessUri.get(), (itr->second)->getURI());
                if (pSmem) {
                    optional<const unsigned char> dir = pSmem.get()->getDir();
                    if (dir) {
                        URI uri = itr->second->getURI();
                        addEndpoint(itr->second, l2Ep, dir.get());
                        l2EpUri.erase(itr);
                    }
                }
            }
        } else {
            // get the list of source member EPGroups.
            // find out if L2Ep is a member of any of these groups.
            // if a match is found, add the L2Ep to the list of sources
            // of the mirror.
            if (!l2Ep->getGroup()) {
                LOG(WARNING) << "EPG has not been set for L2Ep " << l2Ep->getURI();
                return;
            }
            URI egUri(l2Ep->getGroup().get());
            boost::optional<shared_ptr<EpGroup>> epgOpt = getEpgIfPartOfSession(egUri);
            if (epgOpt) {
                std::vector<OF_SHARED_PTR<modelgbp::gbp::EpGroupToSpanSessionRSrc>> vGrpToSess;
                epgOpt->get()->resolveGbpEpGroupToSpanSessionRSrc(vGrpToSess);
                for (auto& sesRsrc : vGrpToSess) {
                    auto it = sess_map.find(sesRsrc->getTargetURI().get());
                    if (it != sess_map.end()) {
                        LOG(DEBUG) << "found session " << sesRsrc->getTargetURI().get();
                        optional<shared_ptr<SrcMember>> srcMem =
                            findSrcMem(sesRsrc->getTargetURI().get(), egUri);
                        if (srcMem) {
                            optional<const unsigned char> dir = srcMem.get()->getDir();
                            if (dir) {
                                SourceEndpoint srcEp(l2Ep->getURI().toString(),
                                                     l2Ep->getInterfaceName().get(),
                                                     dir.get());
                                it->second->addSrcEndpoint(srcEp);
                                notifyUpdate.insert(sesRsrc->getTargetURI().get());
                            }
                        }
                    }
                }
            }
        }
    }

    boost::optional<shared_ptr<EpGroup>> SpanManager::getEpgIfPartOfSession(const URI& epgUri) {
        for (const auto& sess : sess_map) {
            optional<shared_ptr<Session>> session = Session::resolve(framework, sess.first);
            if (!session)
                continue;
            vector <shared_ptr<SrcGrp>> srcGrpVec;
            session.get()->resolveSpanSrcGrp(srcGrpVec);
            for (auto& srcGrp : srcGrpVec) {
                vector<shared_ptr<SrcMember>> srcMemVec;
                srcGrp->resolveSpanSrcMember(srcMemVec);
                for (const shared_ptr<SrcMember>& srcMem : srcMemVec) {
                    optional <shared_ptr<MemberToRefRSrc>> memRefOpt =
                        srcMem->resolveSpanMemberToRefRSrc();
                    if (memRefOpt) {
                        shared_ptr <MemberToRefRSrc> memRef = memRefOpt.get();
                        if (memRef->getTargetClass()) {
                            class_id_t class_id = memRef->getTargetClass().get();
                            if (class_id == modelgbp::gbp::EpGroup::CLASS_ID) {
                                if (memRef->getTargetURI()) {
                                    return EpGroup::resolve(framework, memRef->getTargetURI().get());
                                }
                            }
                        }
                    }
                }
            }
        }
        return boost::none;
    }
}
