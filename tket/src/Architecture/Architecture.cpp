// Copyright 2019-2021 Cambridge Quantum Computing
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Architecture/Architecture.hpp"

#include <boost/graph/biconnected_components.hpp>
#include <unordered_set>
#include <vector>

#include "Graphs/ArticulationPoints.hpp"
#include "Utils/Json.hpp"
#include "Utils/UnitID.hpp"

namespace tket {

Architecture Architecture::create_subarch(
    const std::vector<Node>& subarc_nodes) {
  Architecture subarc(subarc_nodes);
  for (auto [u1, u2] : get_connections_vec()) {
    if (subarc.uid_exists(u1) && subarc.uid_exists(u2)) {
      subarc.add_connection(u1, u2);
    }
  }
  return subarc;
}

unsigned Architecture::get_diameter() const {
  unsigned N = n_uids();
  if (N == 0) {
    throw ArchitectureInvalidity("No nodes in architecture.");
  }
  unsigned max = 0;
  const node_vector_t uids = get_all_uids_vec();
  for (unsigned i = 0; i < N; i++) {
    for (unsigned j = i + 1; j < N; j++) {
      unsigned d = get_distance(uids[i], uids[j]);
      if (d > max) max = d;
    }
  }
  return max;
}

// Given a vector of lengths of lines, returns a vector of lines of these sizes
// comprised of architecture nodes
std::vector<node_vector_t> Architecture::get_lines(
    std::vector<unsigned> required_lengths) const {
  // check total length doesn't exceed number of nodes
  if (std::accumulate(required_lengths.begin(), required_lengths.end(), 0u) >
      n_uids()) {
    throw ArchitectureInvalidity(
        "Not enough nodes to satisfy required lengths.");
  }
  std::sort(
      required_lengths.begin(), required_lengths.end(),
      std::greater<unsigned>());

  UndirectedConnGraph curr_graph(get_undirected_connectivity());
  std::vector<node_vector_t> found_lines;
  for (unsigned length : required_lengths) {
    std::vector<Vertex> longest(
        graphs::longest_simple_path(curr_graph, length));
    if (longest.size() >= length) {
      longest.resize(length);
      // convert Vertex to Node
      node_vector_t to_add;
      for (Vertex v : longest) {
        to_add.push_back(curr_graph[v].uid);
      }
      found_lines.push_back(to_add);
      for (Vertex v : longest) {
        boost::clear_vertex(v, curr_graph);
      }
    }
  }
  return found_lines;
}

std::set<Node> Architecture::get_articulation_points(
    const Architecture& subarc) const {
  return graphs::get_subgraph_aps<Node>(
      get_undirected_connectivity(), subarc.get_undirected_connectivity());
}

std::set<Node> Architecture::get_articulation_points() const {
  std::set<Vertex> aps;
  UndirectedConnGraph undir_g = get_undirected_connectivity();
  boost::articulation_points(undir_g, std::inserter(aps, aps.begin()));
  std::set<Node> ret;
  for (Vertex ap : aps) {
    ret.insert(undir_g[ap].uid);
  }
  return ret;
}

node_set_t Architecture::remove_worst_nodes(unsigned num) {
  node_set_t out;
  Architecture original_arch(*this);
  for (unsigned k = 0; k < num; k++) {
    std::optional<Node> v = find_worst_node(original_arch);
    if (v.has_value()) {
      remove_uid(v.value());
      out.insert(v.value());
    }
  }
  return out;
}

static bool lexicographical_comparison(
    const std::vector<std::size_t>& dist1,
    const std::vector<std::size_t>& dist2) {
  return std::lexicographical_compare(
      dist1.begin(), dist1.end(), dist2.begin(), dist2.end());
}

int tri_lexicographical_comparison(
    const std::vector<std::size_t>& dist1,
    const std::vector<std::size_t>& dist2) {
  // add assertion that these are the same size distance vectors
  auto start_dist1 = dist1.cbegin();
  auto start_dist2 = dist2.cbegin();

  while (start_dist1 != dist1.cend()) {
    if (start_dist2 == dist2.cend() || *start_dist2 < *start_dist1) {
      return 0;
    } else if (*start_dist1 < *start_dist2) {
      return 1;
    }
    ++start_dist1;
    ++start_dist2;
  }
  return -1;
}

MatrixXb Architecture::get_connectivity() const {
  unsigned n = n_uids();
  MatrixXb connectivity = MatrixXb(n, n);
  for (unsigned i = 0; i != n; ++i) {
    for (unsigned j = 0; j != n; ++j) {
      connectivity(i, j) = connection_exists(Node(i), Node(j)) |
                           connection_exists(Node(j), Node(i));
    }
  }
  return connectivity;
}

std::optional<Node> Architecture::find_worst_node(
    const Architecture& original_arch) {
  node_set_t ap = get_articulation_points();
  node_set_t min_nodes = min_degree_uids();

  std::set<Node> bad_nodes;
  std::set_difference(
      min_nodes.cbegin(), min_nodes.cend(), ap.cbegin(), ap.cend(),
      std::inserter(bad_nodes, bad_nodes.begin()));

  if (bad_nodes.empty()) {
    return std::nullopt;
  }

  std::vector<std::size_t> worst_distances, temp_distances;
  Node worst_node = *bad_nodes.begin();
  worst_distances = get_distances(worst_node);
  for (Node temp_node : bad_nodes) {
    temp_distances = get_distances(temp_node);

    int distance_comp =
        tri_lexicographical_comparison(temp_distances, worst_distances);
    if (distance_comp == 1) {
      worst_node = temp_node;
      worst_distances = temp_distances;
    } else if (distance_comp == -1) {
      std::vector<std::size_t> temp_distances_full =
          original_arch.get_distances(temp_node);
      std::vector<std::size_t> worst_distances_full =
          original_arch.get_distances(worst_node);
      if (lexicographical_comparison(
              temp_distances_full, worst_distances_full)) {
        worst_node = temp_node;
        worst_distances = temp_distances;
      }
    }
  }
  return worst_node;
}

void to_json(nlohmann::json& j, const Architecture::Connection& link) {
  j.push_back(link.first);
  j.push_back(link.second);
}

void from_json(const nlohmann::json& j, Architecture::Connection& link) {
  link.first = j.at(0).get<Node>();
  link.second = j.at(1).get<Node>();
}

void to_json(nlohmann::json& j, const Architecture& ar) {
  // Preserve the internal order of ids since Placement depends on this
  auto uid_its = ar.get_all_uids();
  std::vector<Node> nodes{uid_its.begin(), uid_its.end()};
  j["nodes"] = nodes;

  nlohmann::json links;
  for (const Architecture::Connection& con : ar.get_connections_vec()) {
    nlohmann::json entry;
    entry["link"] = con;
    entry["weight"] = ar.get_connection_weight(con.first, con.second);
    links.push_back(entry);
  }
  j["links"] = links;
}

void from_json(const nlohmann::json& j, Architecture& ar) {
  for (const Node& n : j.at("nodes").get<node_vector_t>()) {
    ar.add_uid(n);
  }
  for (const auto& j_entry : j.at("links")) {
    Architecture::Connection l =
        j_entry.at("link").get<Architecture::Connection>();
    double w = j_entry.at("weight").get<double>();
    ar.add_connection(l.first, l.second, w);
  }
}

//////////////////////////////////////
//      Architecture subclasses     //
//////////////////////////////////////
FullyConnected::FullyConnected(unsigned numberOfNodes)
    : Architecture(get_edges(numberOfNodes)) {}

node_vector_t FullyConnected::get_nodes_canonical_order(
    unsigned numberOfNodes) {
  node_vector_t nodes;
  for (unsigned i = 0; i < numberOfNodes; i++) {
    Node n("fcNode", i);
    nodes.push_back(n);
  }
  return nodes;
}

// The node names below ("fcNode" etc) must begin with a lowercase letter to
// match QASM requirements when converting circuits.

std::vector<Architecture::Connection> FullyConnected::get_edges(
    unsigned numberOfNodes) {
  std::vector<Connection> edges;
  for (unsigned i = 0; i < numberOfNodes; i++) {
    for (unsigned j = 0; j < numberOfNodes; j++) {
      if (i != j) {
        Node n1("fcNode", i);
        Node n2("fcNode", j);
        edges.push_back({n1, n2});
      }
    }
  }
  return edges;
}

RingArch::RingArch(unsigned numberOfNodes)
    : Architecture(get_edges(numberOfNodes)) {}

node_vector_t RingArch::get_nodes_canonical_order(unsigned numberOfNodes) {
  node_vector_t nodes;
  for (unsigned i = 0; i < numberOfNodes; i++) {
    Node n("ringNode", i);
    nodes.push_back(n);
  }
  return nodes;
}

std::vector<Architecture::Connection> RingArch::get_edges(
    unsigned numberOfNodes) {
  std::vector<Connection> edges;
  for (unsigned i = 0; i < numberOfNodes; i++) {
    Node n1("ringNode", i);
    Node n2("ringNode", (i + 1) % numberOfNodes);
    edges.push_back({n1, n2});
  }
  return edges;
}

SquareGrid::SquareGrid(
    const unsigned dim_r, const unsigned dim_c, const unsigned _layers)
    : Architecture(get_edges(dim_r, dim_c, _layers)),
      dimension_r{dim_r},
      dimension_c{dim_c},
      layers{_layers} {}

node_vector_t SquareGrid::get_nodes_canonical_order(
    unsigned dim_r, const unsigned dim_c, const unsigned layers) {
  node_vector_t nodes;
  for (unsigned l = 0; l < layers; l++) {
    for (unsigned ver = 0; ver < dim_r; ver++) {
      for (unsigned hor = 0; hor < dim_c; hor++) {
        Node n("gridNode", ver, hor, l);
        nodes.push_back(n);
      }
    }
  }
  return nodes;
}

std::vector<Architecture::Connection> SquareGrid::get_edges(
    const unsigned dim_r, const unsigned dim_c, const unsigned layers) {
  std::vector<Connection> edges;
  for (unsigned l = 0; l < layers; l++) {
    for (unsigned ver = 0; ver < dim_r; ver++) {
      for (unsigned hor = 0; hor < dim_c; hor++) {
        Node n("gridNode", ver, hor, l);
        if (hor != dim_c - 1) {
          Node h_neighbour("gridNode", ver, hor + 1, l);
          edges.push_back({n, h_neighbour});
        }
        if (ver != dim_r - 1) {
          Node v_neighbour("gridNode", ver + 1, hor, l);
          edges.push_back({n, v_neighbour});
        }
        if (l != layers - 1) {
          Node l_neighbour("gridNode", ver, hor, l + 1);
          edges.push_back({n, l_neighbour});
        }
      }
    }
  }
  return edges;
}

}  // namespace tket