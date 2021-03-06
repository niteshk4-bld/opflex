/*
 * Test suite for class NetFlowRenderer
 *
 * Copyright (c) 2020 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <boost/test/unit_test.hpp>

#include <opflexagent/logging.h>
#include <opflexagent/test/BaseFixture.h>
#include <NetFlowRenderer.h>
#include "MockRpcConnection.h"

#include <modelgbp/netflow/CollectorVersionEnumT.hpp>

namespace opflexagent {

using namespace std;
using namespace rapidjson;

BOOST_AUTO_TEST_SUITE(NetFlowRenderer_test)

class NetFlowRendererFixture : public BaseFixture {
public:
    NetFlowRendererFixture() : BaseFixture() {
        nfr = make_shared<NetFlowRenderer>(agent);
        initLogging("debug", false, "");
        conn.reset(new MockRpcConnection());
        nfr->start("br-int", conn.get());
        nfr->connect();
    }

    virtual ~NetFlowRendererFixture() {
        nfr->stop();
    };

    shared_ptr<NetFlowRenderer> nfr;
    unique_ptr<OvsdbConnection> conn;
};

bool verifyCreateDestroy(Agent& agent, const shared_ptr<NetFlowRenderer>& nfr) {
    nfr->setNextId(2000);

    Mutator mutator(agent.getFramework(), "policyreg");
    auto root = modelgbp::dmtree::Root::createRootElement(agent.getFramework());
    auto pu = root->addPolicyUniverse();
    auto platform = pu->addPlatformConfig("platform");
    auto exporterConfig = platform->addNetflowExporterConfig("exporter");
    URI exporterURI = exporterConfig->getURI();

    bool result = nfr->createNetFlow("5.5.5.6", 10);
    ExporterConfigState state(exporterURI, "test");
    state.setVersion(1); // modelgbp::netflow::CollectorVersionEnumT::CONST_V5
    shared_ptr<ExporterConfigState> statePtr = make_shared<ExporterConfigState>(state);
    nfr->exporterDeleted(statePtr);

    result = result && nfr->createIpfix("5.5.5.5", 500);
    statePtr->setVersion(2); // modelgbp::netflow::CollectorVersionEnumT::CONST_V9

    exporterConfig->setDscp(99);
    exporterConfig->setSrcAddr("3.3.3.3");
    exporterConfig->setVersion(2);
    exporterConfig->setDstAddr("5.5.5.7");
    agent.getNetFlowManager().updateExporterConfigState(exporterConfig);
    nfr->exporterUpdated(exporterURI);

    nfr->exporterDeleted(statePtr);
    return result;
}

BOOST_FIXTURE_TEST_CASE( verify_createdestroy, NetFlowRendererFixture ) {
    BOOST_CHECK_EQUAL(true, verifyCreateDestroy(agent, nfr));
}
BOOST_AUTO_TEST_SUITE_END()

}