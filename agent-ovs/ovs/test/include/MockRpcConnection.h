/* -*- C++ -*-; c-basic-offset: 4; indent-tabs-mode: nil */
/*
 * Copyright (c) 2020 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef OPFLEX_MOCKRPCCONNECTION_H
#define OPFLEX_MOCKRPCCONNECTION_H

#include "OvsdbConnection.h"

namespace opflexagent {

/**
 * class for a mockup of an RpcConnection object
 */
class MockRpcConnection : public opflexagent::OvsdbConnection {
public:
    /**
     * constructor that takes a Transaction object reference
     */
    MockRpcConnection() : OvsdbConnection(false) {}

    /**
     * establish mock connection
     */
    virtual void connect() { setConnected(true);}

    /**
     * disconnect mock connection
     */
    virtual void disconnect() { setConnected(false);}

    /**
     * send transaction
     *
     * @param[in] requests list of Transact messages
     * @param[in] trans callback
     */
    virtual void sendTransaction(const list<OvsdbTransactMessage>& requests, Transaction* trans);

    /**
     * destructor
     */
    virtual ~MockRpcConnection() {}
};


/**
 * class holding the request response lookup
 */
class ResponseDict {
public:
    /**
     * get the sole instance of this class
     */
    static ResponseDict& Instance();

    /**
     * initialize the object instance
     */
    void init();

protected:
    /**
     * constructor
     */
    ResponseDict() {}

    /**
     * destructor
     */
    ~ResponseDict() {}
public:
    /**
     * flag to indicate initialization state
     */
    bool isInitialized = false;

    /**
     * map of request response pairs
     */
    map<uint64_t, int> dict;

    /**
     * number of span responses to send
     */
    static const unsigned int no_of_span_msgs = 47;

    /**
     * number of netflow responses to send
     */
    static const unsigned int no_of_netflow_msgs = 14;

    /**
     * rapidjson Document object array
     */
    Document d[no_of_span_msgs + no_of_netflow_msgs];
private:

    /* SPAN request/responses start */

    //string request1 {"[\"Open_vSwitch\",{\"where\":[],\"table\":\"Mirror\",\"op\":\"select\"}]"};
    string selectMirrorResp {"[{\"rows\":[{\"statistics\":[\"map\",[[\"tx_bytes\",0],[\"tx_packets\",0]]],\
            \"_version\":[\"uuid\",\"ec4c165c-335d-477f-a96b-c37c02d6131b\"],\"select_all\"\
             :false,\"name\":\"sess1\",\"output_vlan\":[\"set\",[]],\"select_dst_port\":\
             [\"uuid\",\"0a7a4d65-e785-4674-a219-167391d10c3f\"],\"select_src_port\":[\"set\",\
             [[\"uuid\",\"0a7a4d65-e785-4674-a219-167391d10c3f\"],[\"uuid\",\
             \"373108c7-ce2d-4d46-a419-1654a5bf47ef\"]]],\"external_ids\":[\"map\",[]],\
             \"snaplen\":[\"set\",[]],\"_uuid\":[\"uuid\",\"3f64048e-0abd-4b96-8874-092a527ee80b\"]\
             ,\"output_port\":[\"uuid\",\"fff42dce-44cb-4b6a-8920-dfc32d88ec07\"],\"select_vlan\"\
              :[\"set\",[]]}]}]"};
    //string request2 {"[\"Open_vSwitch\",{\"where\":[],\"table\":\"Port\",\"op\":\"select\",\"columns\":[\"_uuid\",\"name\"]}]"};
    string selectPortsResp {"[{\"rows\":[{\"name\":\"br-int\",\"_uuid\":[\"uuid\",\
            \"ffaee0cd-bb7d-4698-9af1-99f57f9b7081\"]},{\"name\":\"erspan\",\"_uuid\":[\"uuid\",\
            \"fff42dce-44cb-4b6a-8920-dfc32d88ec07\"]},{\"name\":\"p1-tap\",\"_uuid\":[\"uuid\",\
            \"0a7a4d65-e785-4674-a219-167391d10c3f\"]},{\"name\":\"p2-tap\",\"_uuid\":[\"uuid\",\
            \"373108c7-ce2d-4d46-a419-1654a5bf47ef\"]}]}]"};
    /*
    string request3 {"[\"Open_vSwitch\",{\"where\":[[\"name\",\"==\",\"erspan\"]],\"table\":\
            \"Port\",\"op\":\"select\"}]"};
            */
    string response3 {"[{\"rows\":[{\"protected\":false,\"statistics\":[\"map\",[]],\
            \"bond_downdelay\":0,\"name\":\"erspan\",\"mac\":[\"set\",[]],\"fake_bridge\":false,\
            \"trunks\":[\"set\",[]],\"_uuid\":[\"uuid\",\"fff42dce-44cb-4b6a-8920-dfc32d88ec07\"],\
            \"rstp_status\":[\"map\",[]],\"tag\":[\"set\",[]],\"_version\":[\"uuid\",\
            \"bbc91b12-a377-4f1e-b7d4-e6499172baac\"],\"cvlans\":[\"set\",[]],\"bond_updelay\":0,\
            \"bond_active_slave\":[\"set\",[]],\"status\":[\"map\",[]],\"external_ids\":[\"map\",[]],\
            \"other_config\":[\"map\",[]],\"qos\":[\"set\",[]],\"bond_mode\":[\"set\",[]],\
            \"rstp_statistics\":[\"map\",[]],\"vlan_mode\":[\"set\",[]],\"interfaces\":[\"uuid\",\
            \"d05435fa-e35c-4661-8402-f5cfe32ca1f3\"],\"bond_fake_iface\":false,\"lacp\":[\"set\",[]]}]}]"};

    /*
    string request5 {"[\"Open_vSwitch\",{\"where\":[[\"_uuid\",\"==\",\
            [\"uuid\",\"7cb323d7-0215-406d-ae1d-679b72e1f6aa\"]]],\"table\":\"Bridge\",\"op\":\
            \"update\",\"row\":{\"ports\":[\"set\",[[\"uuid\",\
            \"0a7a4d65-e785-4674-a219-167391d10c3f\"],[\"uuid\",\
            \"373108c7-ce2d-4d46-a419-1654a5bf47ef\"],[\"uuid\",\
            \"ffaee0cd-bb7d-4698-9af1-99f57f9b7081\"]]]}}]"};
            */
    string updateBridgePortsResp {"[{\"count\":1}]"};

    string getMirrorUuidResp {"[{\"rows\":[{\"_uuid\":[\"uuid\",\"5167f875-139e-4a62-9147-1170f71b3b4b\"]}]}]"};
    /*
    string request6 {"[\"Open_vSwitch\",{\"where\":[[\"name\",\"==\",\"br-int\"]],\"table\":\
            \"Bridge\",\"op\":\"update\",\"row\":{\"mirrors\":[\"set\",[]]}}]"};
            */
    string deleteMirrorResp {"[{\"count\":1}]"};

    /*
    string request8 {"[\"Open_vSwitch\",{\"uuid-name\":\"rowfaaf29de_8043_4197_8636_4c3d72aeefef\",\
            \"table\":\"Port\",\"op\":\"insert\",\"row\":{\"interfaces\":[\"named-uuid\",\
            \"rowfe66a640_545c_4de1_97d9_a6ea9c0e4434\"],\"name\":\"erspan\"}},{\"uuid-name\":\
            \"rowfe66a640_545c_4de1_97d9_a6ea9c0e4434\",\"table\":\"Interface\",\"op\":\"insert\",\
            \"row\":{\"name\":\"erspan\",\"options\":[\"map\",[[\"erspan_idx\",\"1\"],[\"erspan_ver\",\
            \"1\"],[\"key\",\"1\"],[\"remote_ip\",\"10.30.120.240\"]]],\"type\":\"erspan\"}},\
            {\"where\":[],\"table\":\"Bridge\",\"op\":\"update\",\"row\":{\"ports\":[\"set\",\
            [[\"uuid\",\"0a7a4d65-e785-4674-a219-167391d10c3f\"],[\"uuid\",\
            \"373108c7-ce2d-4d46-a419-1654a5bf47ef\"],[\"uuid\",\"ffaee0cd-bb7d-4698-9af1-\
            99f57f9b7081\"],[\"named-uuid\",\"rowfaaf29de_8043_4197_8636_4c3d72aeefef\"]]]}}]"};
            */
    string interfaceInsertResp {"[{\"uuid\":[\"uuid\",\"67a63d27-9f82-48e6-9931-068bf7dd1b1d\"]},{\"uuid\":[\"uuid\",\
            \"56eadeda-cb76-4d09-b49a-b5abf7640cd4\"]},{\"count\":1}]"};

    /*
    string request13 {"[\"Open_vSwitch\",{\"uuid-name\":\"row8c31e171_96ef_452c_8a4b_abbdf1ea498c\
            \",\"table\":\"Mirror\",\"op\":\"insert\",\"row\":{\"name\":\"msandhu-sess1\",\
            \"output_port\":[\"set\",[[\"uuid\",\"67a63d27-9f82-48e6-9931-068bf7dd1b1d\"]]],\
            \"select_dst_port\":[\"set\",[[\"uuid\",\"0a7a4d65-e785-4674-a219-167391d10c3f\"],\
            [\"uuid\",\"373108c7-ce2d-4d46-a419-1654a5bf47ef\"]]],\"select_src_port\":[\"set\",\
            [[\"uuid\",\"373108c7-ce2d-4d46-a419-1654a5bf47ef\"],[\"uuid\",\"0a7a4d65-e785-4674-\
            a219-167391d10c3f\"]]]}},{\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"7cb323d7-0215-\
            406d-ae1d-679b72e1f6aa\"]]],\"table\":\"Bridge\",\"op\":\"update\",\"row\":\
            {\"mirrors\":[\"named-uuid\",\"row8c31e171_96ef_452c_8a4b_abbdf1ea498c\"]}}]"};
            */
    string createMirrorResp {"[{\"uuid\":[\"uuid\",\"ad0810fb-fa38-4dd0-b0b3-6a98985dd2bc\"]},{\"count\":1}]"};

    string selectInterfaceResp {"[{\"rows\":[{\"options\":[\"map\",[[\"erspan_dir\",\"32552\"],[\"erspan_hwid\",\"2\"],[\"erspan_ver\",\"2\"],[\"key\",\"1\"],[\"remote_ip\",\"11.2.3.4\"]]]}]}]"};

    /* SPAN request/responses end */

    /* NetFlow request/responses start */

    /*
      get bridge uuid
    */
    string getUuidResp {"[{\"rows\":[{\"_uuid\":[\"uuid\",\"7cb323d7-0215-406d-ae1d-679b72e1f6aa\"]}]}]"};

    /*
     delete ipfix/netflow
    */
    string deleteResp{"[{\"count\":1}]"};

    /*
     string createNetflow   {"Open_vSwitch",{"uuid-name":"row5ceec9e1-c8a4-496a-a265-1e98ec2986d1","table":"NetFlow",
                             "op":"insert","row":{"active_timeout":180,"add_id_to_interface":false,"targets":"172.28.184.20:2055"}},
                             {"where":[["_uuid","==",["uuid","18368680-b320-458f-927c-3e8e87a75a7a"]]],"table":"Bridge",
                             "op":"update","row":{"netflow":["named-uuid","row5ceec9e1-c8a4-496a-a265-1e98ec2986d1"]}}]}
    */
    string createNetflowResp{"[{\"uuid\":[\"uuid\",\"8efc3cdd-5504-4943-90a7-06aa15fac286\"]},{\"count\":1}]"};

    /*
     string ipFixReq {"Open_vSwitch",{"uuid-name":"ipfix1","table":"IPFIX","op":"insert","row":{"targets":"172.28.184.9:2055"}},
                      {"where":[["_uuid","==",["uuid","18368680-b320-458f-927c-3e8e87a75a7a"]]],"table":"Bridge","op":"update",
                      "row":{"ipfix":["named-uuid","ipfix1"]}}]}
    */
    string createIpFixResp{"[{\"uuid\":[\"uuid\",\"8a2f834f-1d4c-4624-9da7-3ac13f73e673\"]},{\"count\":1}]"};


    /* NetFlow request/responses end */

    string response[no_of_span_msgs + no_of_netflow_msgs] = {selectMirrorResp, selectPortsResp, response3,
            updateBridgePortsResp, getMirrorUuidResp, deleteMirrorResp, interfaceInsertResp, getUuidResp,
            selectPortsResp, selectPortsResp, selectPortsResp, createMirrorResp, interfaceInsertResp,
            selectPortsResp, updateBridgePortsResp, getMirrorUuidResp, updateBridgePortsResp,
            getUuidResp, updateBridgePortsResp, selectInterfaceResp, selectMirrorResp, selectPortsResp,
            getUuidResp, updateBridgePortsResp, getUuidResp, updateBridgePortsResp, getUuidResp,
            updateBridgePortsResp, getMirrorUuidResp, deleteMirrorResp, getUuidResp, createMirrorResp,
            selectMirrorResp, selectMirrorResp, selectPortsResp, getUuidResp, deleteMirrorResp,
            getUuidResp, deleteMirrorResp, getUuidResp, deleteMirrorResp, interfaceInsertResp, getUuidResp,
            deleteMirrorResp, getUuidResp, createMirrorResp, updateBridgePortsResp,

            deleteResp, deleteResp, getUuidResp, createNetflowResp, deleteResp,
            deleteResp, deleteResp, getUuidResp, createIpFixResp,
            deleteResp, deleteResp, getUuidResp, createIpFixResp, deleteResp};

};

}

#endif //OPFLEX_MOCKRPCCONNECTION_H
