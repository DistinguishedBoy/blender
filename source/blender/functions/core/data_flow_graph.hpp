#pragma once

#include "core.hpp"

#include "BLI_small_set.hpp"

namespace FN {

	class Socket;
	class Node;
	class GraphLinks;
	class DataFlowGraph;

	class Socket {
	public:
		static inline Socket Input(const Node *node, uint index);
		static inline Socket Output(const Node *node, uint index);

		const Node *node() const
		{
			return this->m_node;
		}

		bool is_input() const
		{
			return !this->m_is_output;
		}

		bool is_output() const
		{
			return this->m_is_output;
		}

		uint index() const
		{
			return this->m_index;
		}

		const Type *type() const;
		std::string name() const;

		friend bool operator==(const Socket &a, const Socket &b)
		{
			return (
				a.m_node == b.m_node &&
				a.m_is_output == b.m_is_output &&
				a.m_index == b.m_index);
		}

	private:
		Socket(const Node *node, bool is_output, uint index)
			: m_node(node), m_is_output(is_output), m_index(index) {}

		const Node *m_node;
		const bool m_is_output;
		const uint m_index;
	};

	class Node {
	public:
		Node(const Function &function)
			: m_function(function) {}

		Socket input(uint index) const
		{
			return Socket::Input(this, index);
		}

		Socket output(uint index) const
		{
			return Socket::Output(this, index);
		}

		const Function &function() const
		{
			return this->m_function;
		}

		const Signature &signature() const
		{
			return this->function().signature();
		}

	private:
		const Function &m_function;
	};

	class Link {
	public:
		static Link New(Socket a, Socket b)
		{
			BLI_assert(a.is_input() != b.is_input());
			if (a.is_input()) {
				return Link(b, a);
			}
			else {
				return Link(a, b);
			}
		}

		Socket from() const
		{
			return this->m_from;
		}

		Socket to() const
		{
			return this->m_to;
		}

		friend bool operator==(const Link &a, const Link &b)
		{
			return a.m_from == b.m_from && a.m_to == b.m_to;
		}

	private:
		Link(Socket from, Socket to)
			: m_from(from), m_to(to) {}

		const Socket m_from;
		const Socket m_to;
	};

	class GraphLinks {
	public:
		void insert(Link link)
		{
			Socket from = link.from();
			Socket to = link.to();

			if (!this->m_links.contains(from)) {
				this->m_links.add(from, SmallSet<Socket>());
			}
			if (!this->m_links.contains(to)) {
				this->m_links.add(to, SmallSet<Socket>());
			}

			this->m_links.lookup_ref(from).add(to);
			this->m_links.lookup_ref(to).add(from);
			this->m_all_links.append(Link::New(from, to));
		}

		SmallSet<Socket> get_linked(Socket socket) const
		{
			return this->m_links.lookup(socket);
		}

		SmallVector<Link> all_links() const
		{
			return this->m_all_links;
		}

	private:
		SmallMap<Socket, SmallSet<Socket>> m_links;
		SmallVector<Link> m_all_links;
	};

	class DataFlowGraph {
	public:
		DataFlowGraph() = default;

		~DataFlowGraph()
		{
			for (const Node *node : this->m_nodes) {
				delete node;
			}
		}

		const Node *insert(const Function &function)
		{
			BLI_assert(this->can_modify());
			const Node *node = new Node(function);
			this->m_nodes.add(node);
			return node;
		}

		void link(Socket a, Socket b)
		{
			BLI_assert(this->can_modify());
			BLI_assert(a.node() != b.node());
			BLI_assert(a.is_input() != b.is_input());
			BLI_assert(m_nodes.contains(a.node()));
			BLI_assert(m_nodes.contains(b.node()));

			m_links.insert(Link::New(a, b));
		}

		inline bool can_modify() const
		{
			return !this->frozen();
		}

		inline bool frozen() const
		{
			return this->m_frozen;
		}

		void freeze()
		{
			this->m_frozen = true;
		}

		SmallVector<Link> all_links() const
		{
			return this->m_links.all_links();
		}

		std::string to_dot() const;

	private:
		bool m_frozen = false;
		SmallSet<const Node *> m_nodes;
		GraphLinks m_links;
	};


	/* Some inline functions.
	 * Those can only come after the declaration of other types. */

	inline Socket Socket::Input(const Node *node, uint index)
	{
		BLI_assert(index < node->signature().inputs().size());
		return Socket(node, false, index);
	}

	inline Socket Socket::Output(const Node *node, uint index)
	{
		BLI_assert(index < node->signature().outputs().size());
		return Socket(node, true, index);
	}

} /* namespace FN */