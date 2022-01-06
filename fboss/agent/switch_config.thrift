#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace go neteng.fboss.switch_config
namespace py neteng.fboss.switch_config
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.switch_config
namespace cpp2 facebook.fboss.cfg
namespace go neteng.fboss.switch_config
namespace php fboss_switch_config

include "fboss/agent/if/common.thrift"
include "fboss/agent/if/mpls.thrift"

typedef i64 (cpp.type = "uint64_t") u64

/**
 * Administrative port state. ie "should the port be enabled?"
 */
enum PortState {
  // Deprecated from when this capture administrative and operational state.
  DOWN = 0,

  DISABLED = 1, // Disabled, no signal transmitted
  ENABLED = 2,
}

/**
 * The parser type for a port.
 * This controls how much of the packet header will be parsed.
 */
enum ParserType {
  L2 = 0,
  L3 = 1,
  L4 = 2,
  ALL = 3,
}

enum PfcWatchdogRecoveryAction {
  /**
   * This option intends to recover from PFC deadlock by ignoring
   * PFC and continuing with packet transmit, most common.
   */
  NO_DROP = 0,

  /**
   * Drop the packets when in deadlock mode to help recover.
   */
  DROP = 1,
}

struct PfcWatchdog {
  1: i32 detectionTimeMsecs;
  2: i32 recoveryTimeMsecs;
  3: PfcWatchdogRecoveryAction recoveryAction = NO_DROP;
}

struct PortPfc {
  1: bool tx = false;
  2: bool rx = false;
  3: PortPgConfigName portPgConfigName;
  4: optional PfcWatchdog watchdog;
}

/**
 * The spanning tree state for a VlanPort.
 *
 * This maps to the FocalPoint API's FM_STP_STATE_* constants.
 */
enum SpanningTreeState {
  BLOCKING = 0, // Receive BPDUs only, no traffic
  DISABLED = 1, // Receive nothing
  FORWARDING = 2, // Process BDPUs and traffic
  LEARNING = 3, // Send and receive BDPUs, but no traffic
  LISTENING = 4, // Send and receive BPDUs, but no learning or traffic
}

/**
 * The speed value for a port
 */
enum PortSpeed {
  DEFAULT = 0, // Default for that port defined by HW
  GIGE = 1000, // Gig Ethernet
  XG = 10000, // 10G
  TWENTYG = 20000, // 20G
  TWENTYFIVEG = 25000, // 25G
  FORTYG = 40000, // 40G
  FIFTYG = 50000, // 50G
  HUNDREDG = 100000, // 100G
  TWOHUNDREDG = 200000, // 200G
  FOURHUNDREDG = 400000, // 400G
}

// <speed>_<num_lanes>_<modulation>_<fec>
// <num_lanes>_<modulation>_<fec> is the outmoster layer settings.
enum PortProfileID {
  PROFILE_DEFAULT = 0,
  PROFILE_10G_1_NRZ_NOFEC = 1,
  PROFILE_20G_2_NRZ_NOFEC = 2,
  PROFILE_25G_1_NRZ_NOFEC = 3,
  PROFILE_40G_4_NRZ_NOFEC = 4,
  PROFILE_50G_2_NRZ_NOFEC = 5,
  PROFILE_100G_4_NRZ_NOFEC = 6,
  PROFILE_100G_4_NRZ_CL91 = 7,
  PROFILE_100G_4_NRZ_RS528 = 8,
  PROFILE_200G_4_PAM4_RS544X2N = 9,
  PROFILE_400G_8_PAM4_RS544X2N = 10,
  PROFILE_10G_1_NRZ_NOFEC_COPPER = 11,
  PROFILE_10G_1_NRZ_NOFEC_OPTICAL = 12,
  PROFILE_20G_2_NRZ_NOFEC_COPPER = 13,
  PROFILE_25G_1_NRZ_NOFEC_COPPER = 14,
  PROFILE_25G_1_NRZ_CL74_COPPER = 15,
  PROFILE_25G_1_NRZ_RS528_COPPER = 16,
  PROFILE_40G_4_NRZ_NOFEC_COPPER = 17,
  PROFILE_40G_4_NRZ_NOFEC_OPTICAL = 18,
  PROFILE_50G_2_NRZ_NOFEC_COPPER = 19,
  PROFILE_50G_2_NRZ_CL74_COPPER = 20,
  PROFILE_50G_2_NRZ_RS528_COPPER = 21,
  PROFILE_100G_4_NRZ_RS528_COPPER = 22,
  PROFILE_100G_4_NRZ_RS528_OPTICAL = 23,
  PROFILE_200G_4_PAM4_RS544X2N_COPPER = 24,
  PROFILE_200G_4_PAM4_RS544X2N_OPTICAL = 25,
  PROFILE_400G_8_PAM4_RS544X2N_OPTICAL = 26,
  PROFILE_100G_4_NRZ_CL91_COPPER = 27,
  PROFILE_100G_4_NRZ_CL91_OPTICAL = 28,
  PROFILE_20G_2_NRZ_NOFEC_OPTICAL = 29,
  PROFILE_25G_1_NRZ_NOFEC_OPTICAL = 30,
  PROFILE_50G_2_NRZ_NOFEC_OPTICAL = 31,
  PROFILE_100G_4_NRZ_NOFEC_COPPER = 32,
  // Special profiles for supporting Wedge100 with YV3_T1 rack so that we can
  // use different serdes values for these profiles
  PROFILE_100G_4_NRZ_CL91_COPPER_RACK_YV3_T1 = 33,
  PROFILE_25G_1_NRZ_NOFEC_COPPER_RACK_YV3_T1 = 34,
}

/**
 * The pause setting for a port
 */
struct PortPause {
  1: bool tx = false;
  2: bool rx = false;
}

/**
 * Using this as default is slightly better for generated bindings w/ nullable
 * languages, most notably python. Setting this as default ensures that the
 * attribute is non-null, but defaults both members to false.
*/
const PortPause NO_PAUSE = {"tx": false, "rx": false};

/**
 *  [DEPRECATED] TODO(joseph5wu)
 *  A Range for L4 port range checker
 *  Define a range bewteen [min, max]
 */
struct L4PortRange {
  1: i32 min;
  2: i32 max;
  3: bool invert = false;
}

/**
 *  [DEPRECATED] TODO(joseph5wu)
 *  A Range for packet length
 *  Define a packet length range [min, max]
 */
struct PktLenRange {
  1: i16 min;
  2: i16 max;
  3: bool invert = false;
}

enum IpType {
  ANY = 0,
  IP = 1,
  IP4 = 2,
  IP6 = 3,
}

enum EtherType {
  ANY = 0x0000,
  IPv4 = 0x0800,
  IPv6 = 0x86DD,
  EAPOL = 0x888E,
  MACSEC = 0x88E5,
  LLDP = 0x88CC,
}

struct Ttl {
  1: i16 value;
  2: i16 mask = 0xFF;
}

enum IpFragMatch {
  // not fragment
  MATCH_NOT_FRAGMENTED = 0,
  // first fragment
  MATCH_FIRST_FRAGMENT = 1,
  // not fragment or the first fragment
  MATCH_NOT_FRAGMENTED_OR_FIRST_FRAGMENT = 2,
  // fragment but not the first frament
  MATCH_NOT_FIRST_FRAGMENT = 3,
  // any fragment
  MATCH_ANY_FRAGMENT = 4,
}

struct GreTunnel {
  1: string ip;
  2: optional Ttl ttl;
}

struct SflowTunnel {
  1: string ip;
  2: optional i32 udpSrcPort;
  3: optional i32 udpDstPort;
  4: optional Ttl ttl;
}

/*
 * Either greTunnel or sflowTunnel (but not both) must be set to specify
 * destination IP and encapsulation type.
 * a tunnel source ip can be specified optionally if mirror collector wants
 * a specific IP to be in L3 header
 */
struct MirrorTunnel {
  1: optional GreTunnel greTunnel;
  2: optional SflowTunnel sflowTunnel;
  3: optional string srcIp;
}

union MirrorEgressPort {
  1: string name (py3.name = "name_");
  2: i32 logicalID;
}

/*
 * Mirror destination
 * At least one of egressPort and tunnel must be set
 *
 *  egressPort - name or ID of port from which mirror traffic egresses
 *               required for SPAN
 *               optional for ERSPAN port mirroring
 *  ip - to be deprecated. ip address is now given in "tunnel"
 *  tunnel - configure the tunnel (sflow or gre) used to mirror the packet
 *           to a remote (IPv4) destination.
 */
struct MirrorDestination {
  1: optional MirrorEgressPort egressPort;
  // ip to be deprecated
  2: optional string ip;
  3: optional MirrorTunnel tunnel;
}

// default DSCP marking for mirrored packets
const byte DEFAULT_MIRROR_DSCP = 0;

/*
 * Mirror,
 *   - name, a logical identifier of mirror
 *   - destination, to deliver mirrored traffic
 *   - dscp, mark the mirrored packets with a DSCP marking for QoS
 *           classification. Note: DSCP marking is only 6 bits (0-63),
 *           not a full byte.
 *   - truncate, flag whether mirrored packets should be truncated (if supported
 *               by platform)
 *
 * Mirroring allows replication of data plane traffic to another destination
 * This destination can be specified either as an egress port, an egress port
 * and a tunnel type with remote routable ip (v4) address, or only a tunnel
 * type with remote routable ip (v4) address.
 *
 * If only egress port is specified, then mirrored data-plane traffic egresses
 * the specified port.
 *
 * If egress port & tunnel with remote ip are specified, then mirrored
 * data-plane traffic egresses the specified port and is delivered to
 * destination ip address.
 *
 * If only tunnel with remote ip is specified, then mirrored data-plane traffic
 * is delivered to the destination ip address via any possible egress port.
 *
 * Traffic delivered to remote IP address can be either sFLow or ERSPAN (type 1)
 * encapsulated.
 * The source and destination UDP ports for sFLow can be specified
 * however the header will be a shim header (rather than full v4/v5 header)
 * whose format can be found in BCM DocSafe document 56960-PG104-RDS pg 595.
 * ERSPAN is basically a "GRE" tunnel with protocol type 0x88be.
 * L2 frame is encapsulated in the selected tunnel and delivered to remote
 * routable destination.
 *
 * A traffic to mirror, can be selected using two ways
 *    1) a traffic ingressing or egressing a particular port
 *    2) a traffic ingressing or egressing that meets "acl" criteria,
 *    (this mirrors a traffic ingressing/egressing switch's acl engine)
 *
 *  mirror direction if INGRESS, only ingressing traffic is mirrored
 *  mirror direction if EGRESS, only egressing traffic is mirrored
 *  mirror direction if BOTH, both ingress/egress traffic is mirrored
 *
 * Only four mirror destinations are possible
 *
 * Mirroring should be used with caution and in circumstances to debug traffic
 * issues, as this feature duplicates traffic and so doubles traffic load.
 */
struct Mirror {
  1: string name;
  2: MirrorDestination destination;
  3: byte dscp = DEFAULT_MIRROR_DSCP;
  4: bool truncate = false;
}

/**
 * The action for an access control entry
 */
enum AclActionType {
  DENY = 0,
  PERMIT = 1,
}

/**
 * The look up class for an acl
 */
enum AclLookupClass {
  DST_CLASS_L3_LOCAL_IP4 = 1,
  DST_CLASS_L3_LOCAL_IP6 = 2,

  // Class for DROP ACL
  CLASS_DROP = 9,

  /*
   * RSW Downlink ports connected to MH-NIC setups configure a queue per host
   * using ACLs. These ACLs use following classes.
   */
  CLASS_QUEUE_PER_HOST_QUEUE_0 = 10,
  CLASS_QUEUE_PER_HOST_QUEUE_1 = 11,
  CLASS_QUEUE_PER_HOST_QUEUE_2 = 12,
  CLASS_QUEUE_PER_HOST_QUEUE_3 = 13,
  CLASS_QUEUE_PER_HOST_QUEUE_4 = 14,
  CLASS_QUEUE_PER_HOST_QUEUE_5 = 15,
  CLASS_QUEUE_PER_HOST_QUEUE_6 = 16,
  CLASS_QUEUE_PER_HOST_QUEUE_7 = 17,
  CLASS_QUEUE_PER_HOST_QUEUE_8 = 18,
  CLASS_QUEUE_PER_HOST_QUEUE_9 = 19,
}

enum PacketLookupResultType {
  PACKET_LOOKUP_RESULT_MPLS_NO_MATCH = 1,
}

/**
 * An access control entry
 */
struct AclEntry {
  /**
   * IP addresses with mask. e.g. 192.168.0.0/16. Can be either V4 or V6
   */
  3: optional string srcIp;
  4: optional string dstIp;

  /**
   * [DEPRECATED]
   * L4 port ranges (TCP/UDP)
   */
  5: optional L4PortRange srcL4PortRange_DEPRECATED;
  6: optional L4PortRange dstL4PortRange_DEPRECATED;

  /**
   * IP Protocol. e.g, 6 for TCP
   * Valid value should between 0-255.
   * Note: Should use uint8, but `byte` in thrift is int8, so use i16 instead.
   * Otherwise we will have to use [-127,-1] to represent [128,255], which is
   * not straight-forward at all.
   */
  7: optional i16 proto;

  /**
   * TCP flags(aka Control bits) Bit Map. e.g, 16 for ACK set
   * Starting from the FIN flag which is bit 0 and bit 7 is the CWR bit
   * Valid value should between 0-255.
   */
  8: optional i16 tcpFlagsBitMap;

  /**
   * Physical switch ports.
   */
  10: optional i16 srcPort;
  11: optional i16 dstPort;

  /**
   * [DEPRECATED]
   * Packet length range
   */
  12: optional PktLenRange pktLenRange_DEPRECATED;

  /**
   * Ip fragment
   */
  13: optional IpFragMatch ipFrag;

  /**
   * Icmp type and code
   * Valid value should between 0-255.
   * Code can only be set if type is set.
   * "proto" field must be 1 (icmpv4) or 58 (icmpv6)
   */
  14: optional i16 icmpType;
  15: optional i16 icmpCode;
  /*
  * Match on DSCP values
  */
  16: optional byte dscp;
  /*
   * Identifier used to refer to the ACL
   */
  17: string name;

  18: AclActionType actionType = PERMIT;

  19: optional string dstMac;

  /* Match TTL if IPv4 and HopLimit if IPv6 */
  20: optional Ttl ttl;

  /* IP type (IPv4, IPv6, ARP, MPLS... */
  21: optional IpType ipType;

  /**
   * L4 port (TCP/UDP)
   */
  22: optional i32 l4SrcPort;
  23: optional i32 l4DstPort;

  /*
   * We switched to using more granular lookupClassNeighbor and
   * lookupClassRoute.
   */
  /* dstClassL3 */
  24: optional AclLookupClass lookupClass_DEPRECATED;

  /* dstClassL2 */
  25: optional AclLookupClass lookupClassL2;

  26: optional AclLookupClass lookupClassNeighbor;

  27: optional AclLookupClass lookupClassRoute;

  28: optional PacketLookupResultType packetLookupResult;

  29: optional EtherType etherType;

  30: optional i32 vlanID;
}

enum AclTableActionType {
  PACKET_ACTION = 0,
  COUNTER = 1,
  SET_TC = 2,
  SET_DSCP = 3,
  MIRROR_INGRESS = 4,
  MIRROR_EGRESS = 5,
}

enum AclTableQualifier {
  SRC_IPV6 = 0,
  DST_IPV6 = 1,
  SRC_IPV4 = 2,
  DST_IPV4 = 3,
  L4_SRC_PORT = 4,
  L4_DST_PORT = 5,
  IP_PROTOCOL = 6,
  TCP_FLAGS = 7,
  SRC_PORT = 8,
  OUT_PORT = 9,
  IP_FRAG = 10,
  ICMPV4_TYPE = 11,
  ICMPV4_CODE = 12,
  ICMPV6_TYPE = 13,
  ICMPV6_CODE = 14,
  DSCP = 15,
  DST_MAC = 16,
  IP_TYPE = 17,
  TTL = 18,
  LOOKUP_CLASS_L2 = 19,
  LOOKUP_CLASS_NEIGHBOR = 20,
  LOOKUP_CLASS_ROUTE = 21,
  ETHER_TYPE = 22,
  OUTER_VLAN = 23,
}

struct AclTable {
  1: string name;
  2: i16 priority;
  3: list<AclEntry> aclEntries = [];
  4: list<AclTableActionType> actionTypes = [];
  5: list<AclTableQualifier> qualifiers = [];
}

enum AclStage {
  INGRESS = 0,
  INGRESS_MACSEC = 1,
  EGRESS_MACSEC = 2,
}

struct AclTableGroup {
  1: string name;
  2: list<AclTable> aclTables = [];
  3: AclStage stage = AclStage.INGRESS;
}

/*
 * We only support unicast in FBOSS, but for completeness sake
 */
enum StreamType {
  UNICAST = 0,
  MULTICAST = 1,
  ALL = 2,
}

struct QueueMatchAction {
  1: i16 queueId;
}

struct PacketCounterMatchAction {
  1: string counterName;
}

struct SetDscpMatchAction {
  1: byte dscpValue;
}

enum ToCpuAction {
  /**
  * Send Copy of the packet to CPU, forward original packet as usual.
  */
  COPY = 0,

  /**
   * Redirect original packet to CPU.
   */
  TRAP = 1,
}

enum MacsecFlowPacketAction {
  DROP = 0,
  FORWARD = 1,
  MACSEC_FLOW = 2,
}

struct MacsecFlowAction {
  1: u64 flowId;
  2: MacsecFlowPacketAction action;
}

// Redirect packet to a different nexthop
struct RedirectToNextHopAction {
  // NextHop IP addresses
  1: list<string> nexthops;
}

struct MatchAction {
  1: optional QueueMatchAction sendToQueue;
  2: optional PacketCounterMatchAction packetCounter_DEPRECATED;
  3: optional SetDscpMatchAction setDscp;
  4: optional string ingressMirror;
  5: optional string egressMirror;
  6: optional string counter;
  7: optional ToCpuAction toCpuAction;
  8: optional MacsecFlowAction macsecFlow;
  9: optional RedirectToNextHopAction redirectToNextHop;
}

struct MatchToAction {
  // This references an ACL added to the acls list in the SwitchConfig object
  1: string matcher;
  2: MatchAction action;
}

enum MMUScalingFactor {
  ONE = 0,
  EIGHT = 1,
  ONE_128TH = 2,
  ONE_64TH = 3,
  ONE_32TH = 4,
  ONE_16TH = 5,
  ONE_8TH = 6,
  ONE_QUARTER = 7,
  ONE_HALF = 8,
  TWO = 9,
  FOUR = 10,
}

// This determines how packets are scheduled on a per queue basis
enum QueueScheduling {
  // The round robin runs on all queues set to use WRR for a port
  // Required to set a weight when using this
  WEIGHTED_ROUND_ROBIN = 0,
  // All packets in this type of queue are dequeued before moving on
  // to the next (lower priority) queue
  // This means this can cause starvation on other queues if not
  // configured properly
  STRICT_PRIORITY = 1,
}

// Detection based on average queue length in bytes with two thresholds.
// If the queue length is below the minimum threshold, then never consider the
// queue congested. If the queue length is above the maximum threshold, then
// always consider the queue congested. If the queue length is in between the
// two thresholds, then probabilistically consider the queue congested.
// The probability grows linearly between the two thresholds. That is,
// if we call the minimum threshold m and the maximum threshold M then the
// formula for the probability at queue length m+k is:
// P(m+k) = k/(M-m) for k between 0 and M-m
struct LinearQueueCongestionDetection {
  1: required i32 minimumLength;
  2: required i32 maximumLength;
  3: i32 probability = 100;
}

// Determines when we will consider a queue to be experiencing congestion
// for the purposes of Active Queue Management
union QueueCongestionDetection {
  1: LinearQueueCongestionDetection linear;
}

// Two behaviors are supported for Active Queue Management:
// drops and ECN marking. ECN marking is also conditional on the entity
// entering the queue being an IP packet marked as ECN-enabled. Thus, the
// two booleans can configure the following behavior during congestion:
// earlyDrop false; ecn false: Tail drops for all packets
// earlyDrop true; ecn false: Early drops for all packets
// earlyDrop false; ecn true: Tail drops for ECN-disabled, ECN for ECN-enabled
// earlyDrop true; ecn true: Early drops for ECN-disabled, ECN for ECN-enabled
enum QueueCongestionBehavior {
  EARLY_DROP = 0, // Drop packets before congested queues are totally full using
  // an algorithm like WRED
  ECN = 1, // Mark congestion enabled packets with Congestion Experienced
}

// Configuration for Active Queue Management of a PortQueue.
// Follows the principles outlined in RFC 7567.
struct ActiveQueueManagement {
  // How we answer the question "Is the queue congested?"
  1: required QueueCongestionDetection detection;
  // How we handle packets on queues experiencing congestion
  2: required QueueCongestionBehavior behavior;
}

struct Range {
  1: i32 minimum;
  2: i32 maximum;
}

union PortQueueRate {
  1: Range pktsPerSec;
  2: Range kbitsPerSec;
}

// It is only necessary to define PortQueues for those that you want to
// change settings on
// It is only necessary to define PortQueues for those where you do not want to
// use the default settings.
// Any queues not described by config will use the system defaults of weighted
// round robin with a default weight of 1
struct PortQueue {
  1: required i16 id;
  // We only use unicast in Fabric
  2: required StreamType streamType = StreamType.UNICAST;
  // This value is ignored if STRICT_PRIORITY is chosen
  3: optional i32 weight;
  4: optional i32 reservedBytes;
  5: optional MMUScalingFactor scalingFactor;
  6: required QueueScheduling scheduling;
  7: optional string name;
  // 8: optional i32 length (deprecated)
  /*
   * Refer PortQueueRate which is a generalized version and allows configuring
   * pps as well as kbps.
   */
  9: optional i32 packetsPerSec_DEPRECATED;
  10: optional i32 sharedBytes;
  // Only Unicast queue supports aqms
  11: optional list<ActiveQueueManagement> aqms;
  12: optional PortQueueRate portQueueRate;

  13: optional i32 bandwidthBurstMinKbits;
  14: optional i32 bandwidthBurstMaxKbits;
}

struct DscpQosMap {
  1: i16 internalTrafficClass;
  2: list<byte> fromDscpToTrafficClass;
  3: optional byte fromTrafficClassToDscp;
}

struct ExpQosMap {
  1: i16 internalTrafficClass;
  2: list<byte> fromExpToTrafficClass;
  3: optional byte fromTrafficClassToExp;
}

struct QosMap {
  1: list<DscpQosMap> dscpMaps;
  2: list<ExpQosMap> expMaps;
  3: map<i16, i16> trafficClassToQueueId;
  // for rx PFC, map the incoming PFC pkts
  // priority to the corresponding queueId
  4: optional map<i16, i16> pfcPriorityToQueueId;
  // used for pfc. Incoming traffic class is mapped
  // to given PG
  5: optional map<i16, i16> trafficClassToPgId;
  //  Maps PFC priority
  // to PG id. Used mainly for TX of PFC where
  // PG is mapped to outgoing PFC priority
  6: optional map<i16, i16> pfcPriorityToPgId;
}

struct QosRule {
  1: i16 queueId;
  2: list<i16> dscp = [];
}

struct QosPolicy {
  1: string name;
  2: list<QosRule> rules;
  3: optional QosMap qosMap;
}

struct TrafficPolicyConfig {
  // Order of entries determines priority of acls when applied
  1: list<MatchToAction> matchToAction = [];
  2: optional string defaultQosPolicy;
  3: optional map<i32, string> portIdToQosPolicy;
}

struct PacketRxReasonToQueue {
  1: required PacketRxReason rxReason;
  2: required i16 queueId;
}

struct CPUTrafficPolicyConfig {
  1: optional TrafficPolicyConfig trafficPolicy;
  // TODO(joseph5wu) Will be deprecated once we begin to use rxReasonToQueueList
  2: optional map<PacketRxReason, i16> rxReasonToCPUQueue;
  // Order of entries determines priority of reasons when applied
  3: optional list<PacketRxReasonToQueue> rxReasonToQueueOrderedList;
}

enum PacketRxReason {
  UNMATCHED = 0, // all packets for which there's not a explicit match rule
  ARP = 1,
  DHCP = 2,
  BPDU = 3,
  L3_SLOW_PATH = 4, // L3 slow path processed pkt
  L3_DEST_MISS = 5, // L3 DIP miss
  TTL_1 = 6, // L3UC or IPMC packet with TTL equal to 1
  CPU_IS_NHOP = 7, // The CPU port is the next-hop in the routing table
  NDP = 8, // NDP discovery messages
  LLDP = 9, // LLDP
  ARP_RESPONSE = 10, // ARP RESPONSE
  BGP = 11, // V4 BGP
  BGPV6 = 12, // V6 BGP
  LACP = 13, // LACP, Slow Protocol
  L3_MTU_ERROR = 14, // Packet size exceeds L3 MTU
  MPLS_TTL_1 = 15, // SAI only for MPLS packet with TTL is 1
  MPLS_UNKNOWN_LABEL = 16, // SAI only for MPLS packet with unprogrammed label
  DHCPV6 = 17, // DHCPv6
}

enum PortLoopbackMode {
  NONE = 0,
  PHY = 1,
  MAC = 2,
}

enum LLDPTag {
  PDU_END = 0,
  CHASSIS = 1,
  PORT = 2,
  TTL = 3,
  PORT_DESC = 4,
  SYSTEM_NAME = 5,
  SYSTEM_DESC = 6,
}

// Packets sampled on a port can be sent to CPU or a Mirror Destination
enum SampleDestination {
  CPU = 0,
  MIRROR = 1,
}

const u64 CPU_PORT_LOGICALID = 0;

typedef string PortQueueConfigName

typedef string PortPgConfigName

typedef string BufferPoolConfigName

const i32 DEFAULT_PORT_MTU = 9412;
/**
 * Configuration for a single logical port
 */
struct Port {
  1: i32 logicalID;
  /*
   * Whether the port is enabled.  Set this to ENABLED for normal operation.
   */
  2: PortState state = PortState.DISABLED;
  /*
   * Packets ingressing on this port will be dropped if they are smaller than
   * minFrameSize.  64 bytes is the minimum frame size allowed by ethernet.
   */
  3: i32 minFrameSize = 64;
  /*
   * Packets ingressing on this port will be dropped if they are larger than
   * maxFrameSize.  This should generally match the MTUs for the VLANs that
   * this port belongs to.
   */
  4: i32 maxFrameSize = DEFAULT_PORT_MTU;
  /*
   * This parameter controls how packets ingressing on this port will be
   * parsed.  If set to L2, only the L2 headers will be parsed.  You need to
   * set this to at least L3 to enable routing.  Set this to L4 if you need
   * features that examine TCP/UDP port numbers, such as ECMP.
   */
  5: ParserType parserType;
  /*
   * This parameter must be set to true for packets ingressing on this port to
   * be routed.  If this is set to False, ingress traffic will only be switched
   * at layer 2.
   */
  6: bool routable;
  /*
   * The VLAN ID for untagged packets ingressing on this port.
   */
  7: i32 ingressVlan;
  /**
   * The speed of the interface in mbps.
   * Value 0 means default setting based on the HW and port type.
   */
  8: PortSpeed speed = DEFAULT;

  /**
   * A configurable string describing the name of the port. If this
   * is not set, it will default to 'port-logicalID'
   */
  9: optional string name;

  /**
   * An optional configurable string describing the port.
   */
  10: optional string description;

  /**
   * Refer portQueueConfigName, which offers similar functionality without
   * having to repeat identical queue config across ports.
   */
  12: list<PortQueue> queues_DEPRECATED = [];

  /**
   * pause configuration
   */
  13: PortPause pause = NO_PAUSE;

  /**
   * sFlow sampling rate for ingressing packets.
   * Every 1/sFlowIngressRate ingressing packets will be sampled.
   * 0 indicates no sampling while 1 indicates sampling all packets.
   */
  14: i64 sFlowIngressRate = 0;

  /**
   * sFlow sampling rate for egressing packets.
   * Every 1/sFlowEgressRate egressing packets will be sampled.
   * 0 indicates no sampling while 1 indicates sampling all packets.
   */
  15: i64 sFlowEgressRate = 0;

  /*
   * Setup port in loopback mode
   */
  17: PortLoopbackMode loopbackMode = PortLoopbackMode.NONE;
  /**
   * if port is mirrored?
   */
  18: optional string ingressMirror;
  19: optional string egressMirror;

  /**
   * The LLDP values we expect to receive on this port, to validate
   * against the actual values received from the neighbour.
   *
   * Only LLDP tags with a string representation, or a clear transform to
   * a string (e.g. uint16_t) can be specified.
   *
   * For composite tags, such as Port/Chassis ID, only the string portion
   * is compared.
   */
  20: map<LLDPTag, string> expectedLLDPValues = {};

  /**
   * Specify the destination of sFlow sampling: CPU or Mirror.
   * If sampleDest is specified as MIRROR, sFlowEgressRate must be 0 because
   * egress sampling to mirror destinations is currently unsupported.
   */
  21: optional SampleDestination sampleDest;

  /*
   * Allows overriding default queue config on individual port.
   *
   * if portQueueConfigName is provided,
   *     port queue config = SwitchConfig::portQueueConfigs[portQueueConfigName]
   * else
   *     port queue config = SwitchConfig::defaultPortQueues
   */
  22: optional PortQueueConfigName portQueueConfigName;

  /*
   * If non-empty, host/route entry with this port as egress gets a ClassID from
   * below list. When the list is exhausted, wrap around.
   */
  23: list<AclLookupClass> lookupClasses = [];

  /*
   * The speed profile id for a port.
   * As FBOSS supports more and more speeds and there're a lot of config
   * combinations(like fec, lane_utilizations, medium) occured for those new
   * platforms. So we'll gradually deprecate the `speed` and `fec` fields from
   * Port struct and only use this `profileID` to program the port.
   * Besides, each platform can support different kinds of speed profiles, and
   * it should be defined in the `PlatformConfig`(platform_config.thrift)
   */
  24: PortProfileID profileID = PortProfileID.PROFILE_DEFAULT;

  25: optional PortPfc pfc;

  /*
   * Counter tags associated with this port, useful to support
   * pre-aggregated ODS counters
   */
  26: optional list<string> counterTags;
}

enum LacpPortRate {
  SLOW = 0,
  FAST = 1,
}

enum LacpPortActivity {
  PASSIVE = 0,
  ACTIVE = 1,
}

const i16 DEFAULT_LACP_HOLD_TIMER_MULTIPLIER = 3;

struct AggregatePortMember {
  /**
   * Member ports are identified according to their logicalID, as defined in
   * struct Port.
   */
  1: i32 memberPortID;
  2: i32 priority;
  3: LacpPortRate rate = FAST;
  4: LacpPortActivity activity = ACTIVE;
  5: i16 holdTimerMultiplier = DEFAULT_LACP_HOLD_TIMER_MULTIPLIER;
}

union MinimumCapacity {
  1: byte linkCount;
  2: double linkPercentage;
}

const MinimumCapacity ALL_LINKS = {"linkPercentage": 1};

struct AggregatePort {
  1: i16 key;
  2: string name;
  3: string description;
  4: list<AggregatePortMember> memberPorts;
  5: MinimumCapacity minimumCapacity = ALL_LINKS;

  /*
   * Counter tags associated with this port, useful to support
   * pre-aggregated ODS counters
   */
  6: optional list<string> counterTags;
}

struct Lacp {
  1: string systemID; // = cpuMAC
  2: i32 systemPriority; // = (2 ^ 16) - 1
}

/**
 * Configuration for a VLAN.
 *
 * For simplicity, we use a single list of VLANs for the entire controller
 * process.  The FocalPoint API has an independent list of VLANs for each
 * switch.  The controller will create each VLAN on each configured switch.
 */
struct Vlan {
  1: string name;
  2: i32 id;
  /**
   * Whether or not to enable stats collection for this VLAN in hardware.
   * (The hardware is only able to collect stats on a limited number of VLANs.)
   */
  3: bool recordStats = 1;
  /**
   * Whether packets ingressing on this VLAN should be routed.
   * When this is false, ingressing traffic will only be switched at layer 2.
   *
   * Note that you must also set the routable attribute to true on each port in
   * the vlan.
   */
  5: bool routable;
  /**
   * IP addresses associated with this interface.
   *
   * IP addresses are only useful if the routable flag is set to true for this
   * VLAN.
   */
  6: list<string> ipAddresses;
  /* V4 DHCP relay address */
  7: optional string dhcpRelayAddressV4;
  /* V6 DHCP relay address */
  8: optional string dhcpRelayAddressV6;

  /* Override DHCPv4/6 relayer on a per host basis */
  9: optional map<string, string> dhcpRelayOverridesV4;
  10: optional map<string, string> dhcpRelayOverridesV6;

  /* Interface ID associated with the vlan */
  11: optional i32 intfID;
}

/**
 * A VLAN <--> Port mapping.
 *
 * The specified logical port will be added to the specified VLAN.
 * The VlanPort structure is separate from Port since a single Port may belong
 * to multiple VLANs.
 */
struct VlanPort {
  1: i32 vlanID;
  2: i32 logicalPort;
  3: SpanningTreeState spanningTreeState;
  /**
   * Whether or not to emit the VLAN tag in outgoing ethernet frames.
   * (Primarily useful when there are multiple VLANs on the same port.)
   */
  4: bool emitTags;
}

/**
 * IPv6 NDP configuration settings for an interface
 */
struct NdpConfig {
  /**
   * How often to send out router advertisements, in seconds.
   *
   * If this is 0 or negative, no router advertisements will be sent.
   */
  1: i32 routerAdvertisementSeconds = 0;
  /**
   * The recommended hop limit that hosts should put in outgoing IPv6 packets.
   *
   * This value may be between 0 and 255, inclusive.
   * 0 means unspecified.
   */
  2: i32 curHopLimit = 255;
  /**
   * The router lifetime to send in advertisement messasges.
   *
   * This should generally be larger than the router advertisement interval.
   */
  3: i32 routerLifetime = 1800;
  /**
   * The lifetime to advertise for IPv6 prefixes on this interface.
   */
  4: i32 prefixValidLifetimeSeconds = 2592000;
  /**
   * The preferred lifetime to advertise for IPv6 prefixes on this interface.
   *
   * This must be smaller than prefixValidLifetimeSeconds
   */
  5: i32 prefixPreferredLifetimeSeconds = 604800;
  /**
  * Managed bit setting for router advertisements
  * When set, it indicates that addresses are available via
  * Dynamic Host Configuration Protocol
  */
  6: bool routerAdvertisementManagedBit = 0;
  /**
  * Other bit setting for router advertisements
  * When set, it indicates that other configuration information is
  * available via DHCPv6.  Examples of such information
  * are DNS-related information or information on other
  * servers within the network.
  */
  7: bool routerAdvertisementOtherBit = 0;
}

/**
 * The configuration for an interface
 */
struct Interface {
  1: i32 intfID;
  /** The VRF where the interface belongs to */
  2: i32 routerID = 0;
  /**
   * The VLAN where the interface sits.
   * There shall be maximum one interface per VLAN
   */
  3: i32 vlanID;
  /** The description of the interface */
  4: optional string name;
  /**
   * The MAC address to be used by this interface.
   * If it is not specified here, it is up to the platform
   * to pick up a MAC for it.
   */
  5: optional string mac;
  /**
   * IP addresses with netmask associated with this interface
   * in the format like: 10.0.0.0/8
   */
  6: list<string> ipAddresses;
  /**
   * IPv6 NDP configuration
   */
  7: optional NdpConfig ndp;

  /**
   * MTU size
   */
  8: optional i32 mtu;
  /**
   * is_virtual is set to true for logical interfaces
   * (e.g. loopbacks) which are associated with
   * a reserver vlan. This VLAN has no ports in it
   * and the interface is expected to always be up.
   */
  9: bool isVirtual = 0;
  /**
  * this flag is set to true if we need to
  * disable auto-state feature for SVI
  */
  10: bool isStateSyncDisabled = 0;
}

struct StaticRouteWithNextHops {
  /** The VRF to which the static route belongs to */
  1: i32 routerID = 0;
  /* Prefix in the format like 10.0.0.0/8 */
  2: string prefix;
  /* IP Address next hops like 1.1.1.1 */
  3: list<string> nexthops;
}

struct StaticRouteNoNextHops {
  /** The VRF where the static route belongs to */
  1: i32 routerID = 0;
  /* Prefix in the format like 10.0.0.0/8 */
  2: string prefix;
}

struct StaticIp2MplsRoute {
  /** The VRF where the static route belongs to */
  1: i32 routerID = 0;
  /* Prefix in the format like 10.0.0.0/8 */
  2: string prefix;
  /* mpls next hops with push labels */
  3: list<common.NextHopThrift> nexthops;
}

struct StaticMplsRouteWithNextHops {
  /* Look up MPLS packet based on label */
  1: mpls.MplsLabel ingressLabel;
  /* forward MPLS packet to nexthops */
  2: list<common.NextHopThrift> nexthops;
}

struct StaticMplsRouteNoNextHops {
  1: mpls.MplsLabel ingressLabel;
}

struct SflowCollector {
  1: string ip;
  2: i16 port;
}

enum LoadBalancerID {
  ECMP = 1,
  AGGREGATE_PORT = 2,
}

enum IPv4Field {
  SOURCE_ADDRESS = 1,
  DESTINATION_ADDRESS = 2,
}

enum IPv6Field {
  SOURCE_ADDRESS = 1,
  DESTINATION_ADDRESS = 2,
  FLOW_LABEL = 3,
}

enum TransportField {
  SOURCE_PORT = 1,
  DESTINATION_PORT = 2,
}

enum MPLSField {
  TOP_LABEL = 1,
  SECOND_LABEL = 2,
  THIRD_LABEL = 3,
}

struct Fields {
  1: set<IPv4Field> ipv4Fields;
  2: set<IPv6Field> ipv6Fields;
  3: set<TransportField> transportFields;
  4: set<MPLSField> mplsFields;
}

enum HashingAlgorithm {
  CRC16_CCITT = 1,

  CRC32_LO = 3,
  CRC32_HI = 4,

  CRC32_ETHERNET_LO = 5,
  CRC32_ETHERNET_HI = 6,

  CRC32_KOOPMAN_LO = 7,
  CRC32_KOOPMAN_HI = 8,
}

struct LoadBalancer {
  1: LoadBalancerID id;
  2: Fields fieldSelection;
  3: HashingAlgorithm algorithm;
  // If not set, the seed will be chosen so as to remain constant across process
  // restarts
  4: optional i32 seed;
}

enum CounterType {
  PACKETS = 0,
  BYTES = 1,
}

struct TrafficCounter {
  1: string name;
  2: list<CounterType> types = [PACKETS];
}

enum L2LearningMode {
  HARDWARE = 0,
  SOFTWARE = 1,
}

/*
 * Used for QCM. Weight factors used to compute interest
 * function to evaluate flows which  are source of congestion
 */
enum BurstMonitorWeight {
  FLOW_MAX_RX_BYTES = 0,
  FLOW_SUM_RX_BYTES = 1,
  QUEUE_MAX_BUF_BYTES = 2,
  QUEUE_MAX_SUM_BYTES = 3,
  QUEUE_MAX_DROP_BYTES = 4,
  QUEUE_SUM_DROP_BYTES = 5,
  QUEUE_MAX_RX_BYTES = 6,
  QUEUE_SUM_RX_BYTES = 7,
  MAX_TM_UTIL = 8,
  AVG_TM_UTIL = 9,
  RANDOM_NUM = 10,
  VIEW_ID = 11,
  MAX_VIEW_ID = 12,
  VIEW_ID_THRESHOLD = 13,
  FLOW_MAX_RX_PKTS = 14,
  FLOW_SUM_RX_PKTS = 15,
  QUEUE_MAX_DROP_PKTS = 16,
  QUEUE_SUM_DROP_PKTS = 17,
  QUEUE_MAX_RX_PKTS = 18,
  QUEUE_SUM_RX_PKTS = 19,
  QUEUE_MAX_TX_BYTES = 20,
  QUEUE_SUM_TX_BYTES = 21,
  QUEUE_MAX_TX_PKTS = 22,
  QUEUE_SUM_TX_PKTS = 23,
}

const i16 DEFAULT_QCM_COLLECTOR_DST_PORT = 30000;
const i32 DEFAULT_QCM_AGING_INTERVAL_MSECS = 50;
const i32 DEFAULT_QCM_EXPORT_THRESHOLD = 1;
const i32 DEFAULT_QCM_SCAN_INTERVAL_USECS = 1000;
const i32 DEFAULT_QCM_NUM_FLOWS_TO_CLEAR = 100;
const i32 DEFAULT_QCM_FLOW_LIMIT = 1000;
const i32 DEFAULT_QCM_FLOWS_PER_VIEW = 16;

/* allow configuration of qcm functionality */
struct QcmConfig {
  1: i32 numFlowSamplesPerView = DEFAULT_QCM_FLOWS_PER_VIEW;
  2: i32 flowLimit = DEFAULT_QCM_FLOW_LIMIT;
  3: i32 numFlowsClear = DEFAULT_QCM_NUM_FLOWS_TO_CLEAR;
  4: i32 scanIntervalInUsecs = DEFAULT_QCM_SCAN_INTERVAL_USECS;
  5: i32 exportThreshold = DEFAULT_QCM_EXPORT_THRESHOLD;
  6: map<BurstMonitorWeight, i16> flowWeights = {
    BurstMonitorWeight.RANDOM_NUM: 1,
  };
  7: i32 agingIntervalInMsecs = DEFAULT_QCM_AGING_INTERVAL_MSECS;
  8: string collectorDstIp;
  10: i16 collectorSrcPort;
  // Arbitrary high number to keep the dstPort unique
  11: i16 collectorDstPort = DEFAULT_QCM_COLLECTOR_DST_PORT;
  12: optional i16 collectorDscp;
  13: optional i32 ppsToQcm;
  14: string collectorSrcIp;
  15: list<i32> monitorQcmPortList = [];
  16: map<i32, list<i32>> port2QosQueueIds;
  17: bool monitorQcmCfgPortsOnly = false;
}

struct Neighbor {
  1: i32 vlanID;
  2: string ipAddress;
}

/*
 * Switch specific settings: global to the switch
 */
struct SwitchSettings {
  1: L2LearningMode l2LearningMode = L2LearningMode.HARDWARE;
  2: bool qcmEnable = false;
  3: bool ptpTcEnable = false;

  /**
   * Set L2 Aging to 5 mins by default, same as Arista -
   * https://www.arista.com/en/um-eos/eos-section-19-3-mac-address-table
   *
   * Time to transition L2 from hit -> miss -> removed
   */
  4: i32 l2AgeTimerSeconds = 300;
  5: i32 maxRouteCounterIDs = 0;

  // neighbors to block egress traffic to
  6: list<Neighbor> blockNeighbors = [];
}

// Global buffer pool shared by {port, pgs}
struct BufferPoolConfig {
  1: required i32 sharedBytes;
  2: required i32 headroomBytes;
}

// max PG/port supported
const i16 PORT_PG_VALUE_MAX = 7;
const i16 PFC_PRIORITY_VALUE_MAX = 7;

// Defines PG (priority group) configuration for ports
// This configuration defines the PG buffer settings for given port(s)
struct PortPgConfig {
  1: required i16 id;
  2: optional string name;
  3: optional MMUScalingFactor scalingFactor;
  // Min buffer available to each PG, port
  4: i32 minLimitBytes;
  // Buffer available after XOFF is hit (i.e. after
  // PFC is triggerred). Intent of this buffer is to
  // gurantee lossless operation by absorbing in-flight
  // packets
  5: optional i32 headroomLimitBytes;
  // Offset from XOFF before allowing XON
  6: optional i32 resumeOffsetBytes;
  // global buffer pool as used by this PG
  7: string bufferPoolName;
}

/**
 * The configuration for a switch.
 *
 * This contains all of the hardware-independent configuration for a
 * single switch/router in the network.
 */
struct SwitchConfig {
  1: i64 version = 0;
  2: list<Port> ports;
  3: list<Vlan> vlans = [];
  4: list<VlanPort> vlanPorts = [];
  /*
   * Broadcom chips have the notion of a "default" VLAN.
   * This VLAN must always exist.  It is used as the default VLAN for ingress
   * packets when a default ingress VLAN hasn't been set on a per-port basis.
   * We always set a per-port ingress VLAN, so this setting doesn't really have
   * much effect for us.
   */
  5: i32 defaultVlan;
  6: list<Interface> interfaces = [];
  7: i32 arpTimeoutSeconds = 60;
  8: i32 arpRefreshSeconds = 20;
  9: i32 arpAgerInterval = 5;
  10: bool proactiveArp = 0;
  // The MAC address to use for the switch CPU.
  11: optional string cpuMAC;
  // Static routes with next hops
  12: list<StaticRouteWithNextHops> staticRoutesWithNhops = [];
  // Prefixes for which to drop traffic
  13: list<StaticRouteNoNextHops> staticRoutesToNull = [];
  // Prefixes for which to send traffic to CPU
  14: list<StaticRouteNoNextHops> staticRoutesToCPU = [];
  // List of all ACLs that are available for use by various agent components
  // ACLs declared here can be referenced in other places in order to tie
  // actions to them, e.g. as part of a MatchToAction
  // Only DROP acls are applied directly, all others require being referenced
  // elsewhere in order to be meaningful
  // Ordering of DROP acls define their priority
  15: list<AclEntry> acls = [];
  // Set max number of probes to a sufficiently high value
  // to allow for the cases where
  // a) We are probing and the agent on other end is restarting.
  // b) The other end is having a hard time and responses to ARP/NDP
  // packets are delayed. This is more of a protection mechanism. We
  // saw a case where upstream switches were not able to respond to ARP
  // in time. The underlying cause was that the upstream switch was getting
  // flooded with control plane traffic while ARP traffic was set to goto a low
  // priority queue. This lead to ARP responses to be delayed by upto 50
  // seconds. We have since fixed the issues, but it was decided to add an extra
  // safety measure here to avoid catastrophic failure if such a situation
  // arises again.On vendor devices we set ARP expiry to be as high as 1500
  // seconds.
  16: i32 maxNeighborProbes = 300;
  17: i32 staleEntryInterval = 10;
  18: list<AggregatePort> aggregatePorts = [];
  // What admin distance to use for each potential clientID
  // These mappings map a ClientID to a AdminDistance
  // Predefined values for these can be found at
  // fboss/agent/if/ctrl.thrift
  /*
   * AdminDistance for each clientID. Lower AdminDistance means higher priority.
   * If same prefix is requested for programming from multiple clientID, then
   * clientID with lowest AdminDistance will be programmed.
   * Predefined values for AdminDistance & ClientID can be found at
   * fboss/agent/if/ctrl.thrift
   * NOTE: Ordered as per increasing admin distance i.e most preferred to least
   */
  19: map<i32, i32> clientIdToAdminDistance = {
    2: 0, // INTERFACE_ROUTE
    3: 0, // LINKLOCAL_ROUTE
    1: 1, // STATIC_ROUTE
    786: 10, // OPENR
    0: 20, // BGPD
    700: 255, // STATIC_INTERNAL
  };
  /* Override source IP for DHCP relay packet to the DHCP server */
  20: optional string dhcpRelaySrcOverrideV4;
  21: optional string dhcpRelaySrcOverrideV6;
  /*
   * Override source IP for DHCP reply packet to client host.
   * This IP has to match one of interface IPs, as it is used to determines
   * which interface/VLAN the client host MAC is searched against in order to
   * send the reply to.
   */
  22: optional string dhcpReplySrcOverrideV4;
  23: optional string dhcpReplySrcOverrideV6;
  24: optional TrafficPolicyConfig globalEgressTrafficPolicy_DEPRECATED;
  25: optional string config_version;
  26: list<SflowCollector> sFlowCollectors = [];
  27: optional Lacp lacp;
  28: list<PortQueue> cpuQueues = [];
  29: optional CPUTrafficPolicyConfig cpuTrafficPolicy;
  30: list<LoadBalancer> loadBalancers = [];
  31: optional TrafficPolicyConfig dataPlaneTrafficPolicy;
  32: list<Mirror> mirrors = [];
  33: list<TrafficCounter> trafficCounters = [];
  34: list<QosPolicy> qosPolicies = [];
  35: list<PortQueue> defaultPortQueues = [];
  // Static MPLS routes with next hops
  36: list<StaticMplsRouteWithNextHops> staticMplsRoutesWithNhops = [];
  // MPLS labels for which to drop traffic
  37: list<StaticMplsRouteNoNextHops> staticMplsRoutesToNull = [];
  // MPLS labels for which to send traffic to CPU
  38: list<StaticMplsRouteNoNextHops> staticMplsRoutesToCPU = [];
  // ingress LER routes
  39: list<StaticIp2MplsRoute> staticIp2MplsRoutes = [];

  // Map of named PortQueueConfigs.
  40: map<PortQueueConfigName, list<PortQueue>> portQueueConfigs = {};

  41: SwitchSettings switchSettings;
  42: optional QcmConfig qcmConfig;
  43: optional map<PortPgConfigName, list<PortPgConfig>> portPgConfigs;
  44: optional map<BufferPoolConfigName, BufferPoolConfig> bufferPoolConfigs;
  // aclTableGroup does not need to be a list at this point, as we only expect to
  // support a single group for the foreseeable future. This could be changed to
  // list<AclTableGroup> later if the need arises to support multiple groups.
  45: optional AclTableGroup aclTableGroup;
}
