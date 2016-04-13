/* -*- C++ -*-; c-basic-offset: 4; indent-tabs-mode: nil */
/*
 * Utility functions for flow tables
 *
 * Copyright (c) 2016 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef OVSAGENT_FLOWCONSTANTS_H
#define OVSAGENT_FLOWCONSTANTS_H

#include <stdint.h>
#include <stddef.h>

#include <string>

namespace ovsagent {
namespace flow {

namespace cookie {
/**
 * The cookie used for learn flow entries that are proactively
 * installed
 */
extern const uint64_t PROACTIVE_LEARN;

/**
 * The cookie used for flow entries that are learned reactively.
 */
extern const uint64_t LEARN;

/**
 * The cookie used for flows that direct neighbor discovery
 * packets to the controller
 */
extern const uint64_t NEIGH_DISC;

/**
 * The cookie used for flows that direct DHCPv4 packets to the
 * controller
 */
extern const uint64_t DHCP_V4;

/**
 * The cookie used for flows that direct DHCPv6 packets to the
 * controller
 */
extern const uint64_t DHCP_V6;

/**
 * The cookie used for flows that direct virtual IPv4 announcement
 * packets to the controller
 */
extern const uint64_t VIRTUAL_IP_V4;

/**
 * The cookie used for flows that direct virtual IPv6 announcement
 * packets to the controller
 */
extern const uint64_t VIRTUAL_IP_V6;

/**
 * The cookie used for flows that direct ICMPv4 error messages that
 * require body translation to the controller
 */
extern const uint64_t ICMP_ERROR_V4;

/**
 * The cookie used for flows that direct ICMPv6 error messages that
 * require body translation to the controller
 */
extern const uint64_t ICMP_ERROR_V6;

} // namespace cookie

namespace meta {

/**
 * "Policy applied" bit.  Bypass policy table because policy has
 * already been applied.
 */
extern const uint64_t POLICY_APPLIED;

namespace out {

/**
 * the OUT_MASK specifies 8 bits that indicate the action to take
 * in the output table.  If nothing is set, then the action is to
 * output to the interface in REG7
 */
extern const uint64_t MASK;

/**
 * Resubmit to the first "dest" table with the source registers
 * set to the corresponding values for the EPG in REG7
 */
extern const uint64_t RESUBMIT_DST;

/**
 * Perform "outbound" NAT action and then resubmit with the source
 * EPG set to the mapped NAT EPG
 */
extern const uint64_t NAT;

/**
 * Output to the interface in REG7 but intercept ICMP error
 * replies and overwrite the encapsulated error packet source
 * address with the (rewritten) destination address of the outer
 * packet.
 */
extern const uint64_t REV_NAT;

/**
 * Output to the tunnel destination appropriate for the EPG
 */
extern const uint64_t TUNNEL;

/**
 * Output to the flood group appropriate for the EPG
 */
extern const uint64_t FLOOD;

} // namespace out
} // namespace meta

namespace id {

/**
 * Array containing all ID namespaces
 */
extern const std::string NAMESPACES[];

/**
 * Total number of ID namespaces
 */
extern const size_t NUM_NAMESPACES;

/**
 * ID namespace for endpoint groups
 */
extern const std::string EPG;

/**
 * ID namespace for flood domains
 */
extern const std::string FD;

/**
 * ID namespace for bridge domains
 */
extern const std::string BD;

/**
 * ID namespace for routing domains
 */
extern const std::string RD;

/**
 * ID namespace for contracts
 */
extern const std::string CONTRACT;

/**
 * ID namespace for external networks
 */
extern const std::string EXTNET;

/**
 * ID namespace for subnets
 */
extern const std::string SUBNET;

/**
 * ID namespace for security groups
 */
extern const std::string SECGROUP;

/**
 * ID namespace for security group sets
 */
extern const std::string SECGROUP_SET;

/**
 * ID namespace for endpoints
 */
extern const std::string ENDPOINT;

/**
 * ID namespace for anycast services
 */
extern const std::string SERVICE;

} // namespace id

} // namespace flow
} // namespace ovsagent

#endif /* OVSAGENT_FLOWCONSTANTS_H */