/**
 * Copyright (c) 2014-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <openr/if/gen-cpp2/Lsdb_types.h>
#include <openr/if/gen-cpp2/Network_types.h>

namespace openr {

using LinkStateMetric = uint64_t;

// HoldableValue is the basic building block for ordered FIB programming
// (rfc 6976)
//
// updateValue() will cause the previous value to be held for ttl
// time, where the ttl is chosen based on if the update is an up or down event.
// Subsequent updateValue() calls with the same value are no-ops.
// An update call with a new value when hasHold() is true results in the hold
// being clearted, thus changeing value().
//
// value() will return the held value until decrementTtl() returns true and the
// held value is cleared.

template <class T>
class HoldableValue {
 public:
  explicit HoldableValue(T val);

  const T& value() const;

  bool hasHold() const;

  // these methods return true if the call results in the value changing
  bool decrementTtl();
  bool updateValue(
      T val, LinkStateMetric holdUpTtl, LinkStateMetric holdDownTtl);

 private:
  bool isChangeBringingUp(T val);

  T val_;
  folly::Optional<T> heldVal_;
  LinkStateMetric holdTtl_{0};
};

//
// Why define Link and LinkState? Isn't link state fully captured by something
// like std::unordered_map<std::string, thrift::AdjacencyDatabase>?
// It is, but these classes provide a few major benefits over that simple
// structure:
//
// 1. Only stores bidirectional links. i.e. for a link, the node at both ends
// is advertising the adjancecy
//
// 2. Defines a hash and comparators that operate on the essential property of a
// network link: the tuple: unorderedPair<orderedPair<nodeName, ifName>,
//                                orderedPair<nodeName, ifName>>
//
// 3. For each unique link in the network, holds a single object that can be
// quickly accessed and modified via the nodeName of either end of the link.
//
// 4. Provides useful apis to read and write link state.
//

class Link {
 public:

  Link(
      const std::string& nodeName1,
      const openr::thrift::Adjacency& adj1,
      const std::string& nodeName2,
      const openr::thrift::Adjacency& adj2);

 private:
  const std::string n1_, n2_, if1_, if2_;
  LinkStateMetric metric1_{1}, metric2_{1};
  int32_t adjLabel1_{0}, adjLabel2_{0};
  bool overload1_{false}, overload2_{false};
  thrift::BinaryAddress nhV41_, nhV42_, nhV61_, nhV62_;
  const std::pair<
      std::pair<std::string, std::string>,
      std::pair<std::string, std::string>>
      orderedNames;

 public:
  const size_t hash{0};

  const std::string& getOtherNodeName(const std::string& nodeName) const;

  const std::string& firstNodeName() const;

  const std::string& secondNodeName() const;

  const std::string& getIfaceFromNode(const std::string& nodeName) const;

  LinkStateMetric getMetricFromNode(const std::string& nodeName) const;

  int32_t getAdjLabelFromNode(const std::string& nodeName) const;

  bool getOverloadFromNode(const std::string& nodeName) const;

  bool isOverloaded() const;

  const thrift::BinaryAddress& getNhV4FromNode(
      const std::string& nodeName) const;

  const thrift::BinaryAddress& getNhV6FromNode(
      const std::string& nodeName) const;

  void setNhV4FromNode(
      const std::string& nodeName, const thrift::BinaryAddress& nhV4);

  void setNhV6FromNode(
      const std::string& nodeName, const thrift::BinaryAddress& nhV6);

  void setMetricFromNode(const std::string& nodeName, LinkStateMetric d);

  void setAdjLabelFromNode(const std::string& nodeName, int32_t adjLabel);

  void setOverloadFromNode(const std::string& nodeName, bool overload);

  bool operator<(const Link& other) const;

  bool operator==(const Link& other) const;

  std::string toString() const;

  std::string directionalToString(const std::string& fromNode) const;
}; // class Link

class LinkState {
 public:
  struct LinkPtrHash {
    bool operator()(const std::shared_ptr<Link>& l) const;
  };

  struct LinkPtrLess {
    bool operator()(
        const std::shared_ptr<Link>& lhs,
        const std::shared_ptr<Link>& rhs) const;
  };

  struct LinkPtrEqual {
    bool operator()(
        const std::shared_ptr<Link>& lhs,
        const std::shared_ptr<Link>& rhs) const;
  };

  using LinkSet =
      std::unordered_set<std::shared_ptr<Link>, LinkPtrHash, LinkPtrEqual>;

  void addLink(std::shared_ptr<Link> link);

  void removeLink(std::shared_ptr<Link> link);

  void removeLinksFromNode(const std::string& nodeName);

  const LinkSet& linksFromNode(const std::string& nodeName) const;

  std::vector<std::shared_ptr<Link>> orderedLinksFromNode(
      const std::string& nodeName);

  bool updateNodeOverloaded(const std::string& nodeName, bool isOverloaded);

  bool isNodeOverloaded(const std::string& nodeName) const;

 private:
  // this stores the same link object accessible from either nodeName
  std::unordered_map<std::string /* nodeName */, LinkSet> linkMap_;

  std::unordered_map<std::string /* nodeName */, bool> nodeOverloads_;

}; // class LinkState
} // namespace openr

namespace std {

// needed for certain containers

template <>
struct hash<openr::Link> {
  size_t operator()(openr::Link const& link) const;
};

} // namespace std
