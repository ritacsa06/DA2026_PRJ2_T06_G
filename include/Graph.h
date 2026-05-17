// Original code by Gonçalo Leão
// Updated by DA 2024/2025 Team

#ifndef DA_TP_CLASSES_GRAPH
#define DA_TP_CLASSES_GRAPH

#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <algorithm>

template <class T>
class Edge;

#define INF std::numeric_limits<double>::max()

/************************* Vertex  **************************/

/**
 * @brief Represents a single vertex in the graph.
 * @tparam T The type of information stored in the vertex (e.g., Web).
 */
template <class T>
class Vertex {
public:
    /**
     * @brief Constructs a new Vertex with the given information.
     * <b>Time Complexity:</b> O(1).
     * @param in The information/content of the vertex.
     */
    Vertex(T in);

    /**
     * @brief Compares this vertex with another based on distance.
     * <b>Time Complexity:</b> O(1).
     * @param vertex The other vertex to compare against.
     * @return True if this vertex's distance is less than the other's.
     */
    bool operator<(Vertex<T> & vertex) const; 

    // Getters and Setters
    T getInfo() const;
    std::vector<Edge<T> *> getAdj() const;
    bool isVisited() const;
    bool isProcessing() const;
    unsigned int getIndegree() const;
    double getDist() const;
    Edge<T> *getPath() const;
    std::vector<Edge<T> *> getIncoming() const;

    void setInfo(T info);
    void setVisited(bool visited);
    void setProcessing(bool processing);

    int getLow() const;
    void setLow(int value);
    int getNum() const;
    void setNum(int value);

    void setIndegree(unsigned int indegree);
    void setDist(double dist);
    void setPath(Edge<T> *path);

    /**
     * @brief Adds an outgoing edge from this vertex to a destination vertex.
     * <b>Time Complexity:</b> O(1).
     * @param dest Pointer to the destination vertex.
     * @param w Weight of the edge.
     * @return Pointer to the newly created edge.
     */
    Edge<T> * addEdge(Vertex<T> *dest, double w);

    /**
     * @brief Removes an outgoing edge targeting a specific content.
     * <b>Time Complexity:</b> O(E_out), where E_out is the number of outgoing edges from this vertex.
     * @param in The content of the destination vertex to remove the edge to.
     * @return True if the edge was found and removed, false otherwise.
     */
    bool removeEdge(T in);

    /**
     * @brief Removes all outgoing edges from this vertex.
     * <b>Time Complexity:</b> O(E_out), where E_out is the number of outgoing edges.
     */
    void removeOutgoingEdges();

protected:
    T info;                      ///< Information stored in the vertex
    std::vector<Edge<T> *> adj;  ///< Outgoing edges

    // Auxiliary fields
    bool visited = false;        ///< Used by DFS, BFS, Prim, etc.
    bool processing = false;     ///< Used by isDAG
    int low = -1, num = -1;      ///< Used by Tarjan's SCC algorithm
    unsigned int indegree;       ///< Used by topological sort
    double dist = 0;             ///< Distance metric for shortest path algorithms
    Edge<T> *path = nullptr;     ///< Path pointer for shortest path algorithms

    std::vector<Edge<T> *> incoming; ///< Incoming edges

    int queueIndex = 0;          ///< Required by MutablePriorityQueue and UFDS

    /**
     * @brief Helper function to delete an edge and remove it from the destination's incoming list.
     * * <b>Time Complexity:</b> O(E_in), where E_in is the number of incoming edges of the destination vertex.
     * @param edge The edge to delete.
     */
    void deleteEdge(Edge<T> *edge);
};

/********************** Edge  ****************************/

/**
 * @brief Represents a directed edge connecting two vertices.
 * @tparam T The type of information stored in the vertices.
 */
template <class T>
class Edge {
public:
    /**
     * @brief Constructs an edge between an origin and destination vertex.
     * <b>Time Complexity:</b> O(1).
     * @param orig Pointer to the origin vertex.
     * @param dest Pointer to the destination vertex.
     * @param w Weight of the edge.
     */
    Edge(Vertex<T> *orig, Vertex<T> *dest, double w);

    // Getters and Setters
    Vertex<T> * getDest() const;
    double getWeight() const;
    bool isSelected() const;
    Vertex<T> * getOrig() const;
    Edge<T> *getReverse() const;
    double getFlow() const;

    void setSelected(bool selected);
    void setReverse(Edge<T> *reverse);
    void setFlow(double flow);
protected:
    Vertex<T> * dest;         ///< Destination vertex
    double weight;            ///< Edge weight (or capacity)

    bool selected = false;    ///< Auxiliary field for MST, etc.

    Vertex<T> *orig;          ///< Origin vertex
    Edge<T> *reverse = nullptr; ///< Pointer to the reverse edge (for bidirectional graphs)

    double flow;              ///< Flow value (for network flow problems)
};

/********************** Graph  ****************************/

/**
 * @brief Represents a generic graph data structure.
 * @tparam T The type of information stored in the vertices.
 */
template <class T>
class Graph {
public:
    /**
     * @brief Graph destructor. Cleans up dynamically allocated matrices and vertices.
     * <b>Time Complexity:</b> O(V + E), where V is the number of vertices and E is the number of edges.
     */
    ~Graph();

    /**
     * @brief Finds a vertex with the given content.
     * <b>Time Complexity:</b> O(V), where V is the number of vertices.
     * @param in The content to search for.
     * @return Pointer to the found vertex, or nullptr if not found.
     */
    Vertex<T> *findVertex(const T &in) const;

    /**
     * @brief Adds a new vertex to the graph.
     * <b>Time Complexity:</b> O(V) due to the existence check.
     * @param in The content of the new vertex.
     * @return True if successful, false if a vertex with that content already exists.
     */
    bool addVertex(const T &in);

    /**
     * @brief Removes a vertex and all its incoming/outgoing edges.
     * <b>Time Complexity:</b> O(V + E), where V is the number of vertices and E is the number of edges.
     * @param in The content of the vertex to remove.
     * @return True if successfully removed, false if the vertex does not exist.
     */
    bool removeVertex(const T &in);

    /**
     * @brief Adds a directed edge between two vertices.
     * <b>Time Complexity:</b> O(V) to locate both vertices.
     * @param sourc The content of the source vertex.
     * @param dest The content of the destination vertex.
     * @param w Weight of the new edge.
     * @return True if successful, false if either vertex does not exist.
     */
    bool addEdge(const T &sourc, const T &dest, double w);

    /**
     * @brief Removes an edge from the graph.
     * <b>Time Complexity:</b> O(V + E_out) where E_out is the out-degree of the source vertex.
     * @param sourc The content of the source vertex.
     * @param dest The content of the destination vertex.
     * @return True if successful, false if the edge or source vertex does not exist.
     */
    bool removeEdge(const T &sourc, const T &dest);

    /**
     * @brief Adds a bidirectional edge between two vertices (two directed edges).
     * <b>Time Complexity:</b> O(V) to locate both vertices.
     * @param sourc The content of one vertex.
     * @param dest The content of the other vertex.
     * @param w Weight of both edges.
     * @return True if successful, false if either vertex does not exist.
     */
    bool addBidirectionalEdge(const T &sourc, const T &dest, double w);

    int getNumVertex() const;
    std::vector<Vertex<T> *> getVertexSet() const;

protected:
    std::vector<Vertex<T> *> vertexSet;    ///< Vector storing all vertices in the graph

    double ** distMatrix = nullptr;   ///< Distance matrix for Floyd-Warshall
    int **pathMatrix = nullptr;       ///< Path matrix for Floyd-Warshall

    /**
     * @brief Finds the index of the vertex with a given content.
     * <b>Time Complexity:</b> O(V).
     * @param in The content to search for.
     * @return The index of the vertex, or -1 if not found.
     */
    int findVertexIdx(const T &in) const;
};

void deleteMatrix(int **m, int n);
void deleteMatrix(double **m, int n);

/************************* Vertex Implementation **************************/

template <class T>
Vertex<T>::Vertex(T in): info(in) {}

template <class T>
Edge<T> * Vertex<T>::addEdge(Vertex<T> *d, double w) {
    auto newEdge = new Edge<T>(this, d, w);
    adj.push_back(newEdge);
    d->incoming.push_back(newEdge);
    return newEdge;
}

template <class T>
bool Vertex<T>::removeEdge(T in) {
    bool removedEdge = false;
    auto it = adj.begin();
    while (it != adj.end()) {
        Edge<T> *edge = *it;
        Vertex<T> *dest = edge->getDest();
        if (dest->getInfo() == in) {
            it = adj.erase(it);
            deleteEdge(edge);
            removedEdge = true; 
        }
        else {
            it++;
        }
    }
    return removedEdge;
}

template <class T>
void Vertex<T>::removeOutgoingEdges() {
    auto it = adj.begin();
    while (it != adj.end()) {
        Edge<T> *edge = *it;
        it = adj.erase(it);
        deleteEdge(edge);
    }
}

template <class T>
bool Vertex<T>::operator<(Vertex<T> & vertex) const {
    return this->dist < vertex.dist;
}

template <class T>
T Vertex<T>::getInfo() const { return this->info; }

template <class T>
int Vertex<T>::getLow() const { return this->low; }

template <class T>
void Vertex<T>::setLow(int value) { this->low = value; }

template <class T>
int Vertex<T>::getNum() const { return this->num; }

template <class T>
void Vertex<T>::setNum(int value) { this->num = value; }

template <class T>
std::vector<Edge<T>*> Vertex<T>::getAdj() const { return this->adj; }

template <class T>
bool Vertex<T>::isVisited() const { return this->visited; }

template <class T>
bool Vertex<T>::isProcessing() const { return this->processing; }

template <class T>
unsigned int Vertex<T>::getIndegree() const { return this->indegree; }

template <class T>
double Vertex<T>::getDist() const { return this->dist; }

template <class T>
Edge<T> *Vertex<T>::getPath() const { return this->path; }

template <class T>
std::vector<Edge<T> *> Vertex<T>::getIncoming() const { return this->incoming; }

template <class T>
void Vertex<T>::setInfo(T in) { this->info = in; }

template <class T>
void Vertex<T>::setVisited(bool visited) { this->visited = visited; }

template <class T>
void Vertex<T>::setProcessing(bool processing) { this->processing = processing; }

template <class T>
void Vertex<T>::setIndegree(unsigned int indegree) { this->indegree = indegree; }

template <class T>
void Vertex<T>::setDist(double dist) { this->dist = dist; }

template <class T>
void Vertex<T>::setPath(Edge<T> *path) { this->path = path; }

template <class T>
void Vertex<T>::deleteEdge(Edge<T> *edge) {
    Vertex<T> *dest = edge->getDest();
    auto it = dest->incoming.begin();
    while (it != dest->incoming.end()) {
        if ((*it)->getOrig()->getInfo() == info) {
            it = dest->incoming.erase(it);
        }
        else {
            it++;
        }
    }
    delete edge;
}

/********************** Edge Implementation ****************************/

template <class T>
Edge<T>::Edge(Vertex<T> *orig, Vertex<T> *dest, double w): dest(dest), weight(w), orig(orig) {}

template <class T>
Vertex<T> * Edge<T>::getDest() const { return this->dest; }

template <class T>
double Edge<T>::getWeight() const { return this->weight; }

template <class T>
Vertex<T> * Edge<T>::getOrig() const { return this->orig; }

template <class T>
Edge<T> *Edge<T>::getReverse() const { return this->reverse; }

template <class T>
bool Edge<T>::isSelected() const { return this->selected; }

template <class T>
double Edge<T>::getFlow() const { return flow; }

template <class T>
void Edge<T>::setSelected(bool selected) { this->selected = selected; }

template <class T>
void Edge<T>::setReverse(Edge<T> *reverse) { this->reverse = reverse; }

template <class T>
void Edge<T>::setFlow(double flow) { this->flow = flow; }

/********************** Graph Implementation ****************************/

template <class T>
int Graph<T>::getNumVertex() const { return vertexSet.size(); }

template <class T>
std::vector<Vertex<T> *> Graph<T>::getVertexSet() const { return vertexSet; }

template <class T>
Vertex<T> * Graph<T>::findVertex(const T &in) const {
    for (auto v : vertexSet)
        if (v->getInfo() == in)
            return v;
    return nullptr;
}

template <class T>
int Graph<T>::findVertexIdx(const T &in) const {
    for (unsigned i = 0; i < vertexSet.size(); i++)
        if (vertexSet[i]->getInfo() == in)
            return i;
    return -1;
}

template <class T>
bool Graph<T>::addVertex(const T &in) {
    if (findVertex(in) != nullptr)
        return false;
    vertexSet.push_back(new Vertex<T>(in));
    return true;
}

template <class T>
bool Graph<T>::removeVertex(const T &in) {
    for (auto it = vertexSet.begin(); it != vertexSet.end(); it++) {
        if ((*it)->getInfo() == in) {
            auto v = *it;
            v->removeOutgoingEdges();
            for (auto u : vertexSet) {
                u->removeEdge(v->getInfo());
            }
            vertexSet.erase(it);
            delete v;
            return true;
        }
    }
    return false;
}

template <class T>
bool Graph<T>::addEdge(const T &sourc, const T &dest, double w) {
    auto v1 = findVertex(sourc);
    auto v2 = findVertex(dest);
    if (v1 == nullptr || v2 == nullptr)
        return false;
    v1->addEdge(v2, w);
    return true;
}

template <class T>
bool Graph<T>::removeEdge(const T &sourc, const T &dest) {
    Vertex<T> * srcVertex = findVertex(sourc);
    if (srcVertex == nullptr) {
        return false;
    }
    return srcVertex->removeEdge(dest);
}

template <class T>
bool Graph<T>::addBidirectionalEdge(const T &sourc, const T &dest, double w) {
    auto v1 = findVertex(sourc);
    auto v2 = findVertex(dest);
    if (v1 == nullptr || v2 == nullptr)
        return false;
    auto e1 = v1->addEdge(v2, w);
    auto e2 = v2->addEdge(v1, w);
    e1->setReverse(e2);
    e2->setReverse(e1);
    return true;
}

inline void deleteMatrix(int **m, int n) {
    if (m != nullptr) {
        for (int i = 0; i < n; i++)
            if (m[i] != nullptr)
                delete [] m[i];
        delete [] m;
    }
}

inline void deleteMatrix(double **m, int n) {
    if (m != nullptr) {
        for (int i = 0; i < n; i++)
            if (m[i] != nullptr)
                delete [] m[i];
        delete [] m;
    }
}

template <class T>
Graph<T>::~Graph() {
    deleteMatrix(distMatrix, vertexSet.size());
    deleteMatrix(pathMatrix, vertexSet.size());

    for (auto v : vertexSet) {
        
        for (auto e : v->getAdj()) {
            delete e; 
        }
      
        delete v;
    }

    
}

#endif /* DA_TP_CLASSES_GRAPH */