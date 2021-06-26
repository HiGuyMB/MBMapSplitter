// Copyright (c) 2014 Whirligig Studios
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// AABB DSATUR Coloring
// Created by Whirligig231, http://www.whirligig231.com/

// The purpose of this code is as follows: it will accept a 2D array of AABB coordinates.
// It then returns a (not necessarily minimal but hopefully close) partition of the AABB indices.
// The resulting partition is guaranteed to have the property that no two AABBs in a group intersect.
// Created to assist with various 3D model exporting programs, which might fail or produce errors
// if multiple objects intersect. With this program, one can produce groups of meshes that have no
// intersections, then combine them some safer way.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <string>
#include <ctime>
#include "aabbcolor.h"

using namespace std;

// A private method to remove all of a given node from the neighbors list.

void GraphNode::vectorRemove(GraphNode *node) {
	this->neighbors.erase(remove(this->neighbors.begin(),this->neighbors.end(),node),this->neighbors.end());
}

// A node is always created with an index, no color (-1 is a special value), and no neighbors.

GraphNode::GraphNode(int index) {
	this->index = index;
	this->color = -1;
	this->neighbors.clear(); // Technically unnecessary, but put here just as a reminder.
}

// Returns the index of this node.

int GraphNode::getIndex() {
	return this->index;
}

// Sets the index of this node. (Should be unnecessary for this application, but who knows.)

void GraphNode::setIndex(int index) {
	this->index = index;
}

// Returns the color of this node. Colors do not mean actual colors, but instead are simply non-negative
// integer color indices. -1 means no color.

int GraphNode::getColor() {
	return this->color;
}

// Sets the color of this node.

void GraphNode::setColor(int color) {
	this->color = color;
}

// Adds a neighbor/edge linking two vertices. This operation *is* symmetric, so that this will also add
// this node as a neighbor to the other node. Returns the new degree.

int GraphNode::addNeighbor(GraphNode *neighbor) {
	if (!this->isNeighbor(neighbor))
		this->neighbors.push_back(neighbor);
	if (!neighbor->isNeighbor(this))
		neighbor->neighbors.push_back(this);
	return this->getDegree();
}

// Removes a neighbor/edge. Again, this will disconnect both vertices from each other. Returns the new
// degree (which can be checked before/after to see if the operation succeeded).

int GraphNode::removeNeighbor(GraphNode *neighbor) {
	this->vectorRemove(neighbor);
	neighbor->vectorRemove(this);
	return this->getDegree();
}

// Returns whether the given vertex is a neighbor of this vertex.

bool GraphNode::isNeighbor(GraphNode *neighbor) {
	return (find(this->neighbors.begin(),this->neighbors.end(),neighbor) != this->neighbors.end());
}

// Returns the degree (number of neighbors) of this vertex.

int GraphNode::getDegree() {
	return (int)this->neighbors.size();
}

// Returns the saturation of this vertex. This is the total number of unique colors used by the
// neighbors of this vertex. Nodes with no color (color < 0) are not counted.

int GraphNode::getSaturation() {
	set<int> uniqueColors;
	vector<GraphNode*>::iterator it;
	for (it = this->neighbors.begin(); it != this->neighbors.end(); it++) {
		int color = (*it)->getColor();
		if (color < 0)
			continue;
		if (uniqueColors.find(color) != uniqueColors.end())
			continue;
		uniqueColors.insert(color);
	}
	return (int)uniqueColors.size();
}

// Returns whether a color is valid for this vertex (none of its neighbors have it).

bool GraphNode::isValidColor(int color) {
	vector<GraphNode*>::iterator it;
	for (it = this->neighbors.begin(); it != this->neighbors.end(); it++) {
		int neighborColor = (*it)->getColor();
		if (neighborColor < 0)
			continue;
		if (neighborColor == color)
			return false;
	}
	return true;
}

// Creates a graph with no vertices.

Graph::Graph() {
	this->nodes.clear(); // Again, just for readability.
}

// Adds a vertex to a graph. The vertex is created here. Returns the new size.

int Graph::addNode(int index) {
	if (!this->containsNode(index)) {
		GraphNode newNode(index);
		this->nodes.push_back(newNode);
	}
	return this->getSize();
}

// Removes a vertex from a graph, either by index or by pointer. Returns the new size.

int Graph::removeNode(int index) {
	vector<GraphNode>::iterator it;
	for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
		if (it->getIndex() == index)
			it = this->nodes.erase(it);
	}
	return this->getSize();
}

// I feel like there's a really easy way to do this that I'm not realizing ...
// Eh, whatever.

int Graph::removeNode(GraphNode *node) {
	vector<GraphNode>::iterator it;
	for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
		if (&(*it) == node)
			it = this->nodes.erase(it);
	}
	return this->getSize();
}

// Returns whether the given node exists in the given graph.

bool Graph::containsNode(int index) {
	vector<GraphNode>::iterator it;
	for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
		if (it->getIndex() == index)
			return true;
	}
	return false;
}

// Returns a reference pointer to the node having the given index. Returns NULL if it doesn't exist.

GraphNode *Graph::findNode(int index) {
	vector<GraphNode>::iterator it;
	for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
		if (it->getIndex() == index)
			return &(*it); // Technically not always "safe," but should be in this context.
	}
	return NULL;
}

// Returns the size of a graph (number of nodes).

int Graph::getSize() {
	return (int)this->nodes.size();
}

// Adds an edge between two vertices.

void Graph::addEdge(int index1, int index2) {
	this->findNode(index1)->addNeighbor(this->findNode(index2));
}

// Removes an edge between two vertices.

void Graph::removeEdge(int index1, int index2) {
	this->findNode(index1)->removeNeighbor(this->findNode(index2));
}

// Returns whether there is an edge between two vertices.

bool Graph::isEdge(int index1, int index2) {
	cout << endl << "ptr " << this->findNode(index1) << " and " << this->findNode(index2) << " -> " << this->findNode(index1)->isNeighbor(this->findNode(index2));
	return this->findNode(index1)->isNeighbor(this->findNode(index2));
}

// Returns the total number of edges in the graph.

int Graph::getEdgeCount() {
	int totalDegree = 0;
	vector<GraphNode>::iterator it;
	for (it = this->nodes.begin(); it != this->nodes.end(); it++)
		totalDegree += it->getDegree();
	return totalDegree/2;
}

// Clears all vertex colors and attempts to find a minimal coloring using the DSATUR algorithm.

void Graph::colorDSATUR() {
	// Remove all vertex colors.
	vector<GraphNode>::iterator it;
	for (it = this->nodes.begin(); it != this->nodes.end(); it++)
		it->setColor(-1);
	// Next, we want to iterate until all nodes are colored.
	while (true) { // We'll break, don't worry.
		GraphNode *next = NULL; // This will be the vertex we operate on.
		// We pick as follows: an uncolored node with the highest saturation.
		// In the case of a tie, choose the node with the highest degree.
		// If a tie still exists, we will choose the first such node in the vector.
		for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
			if (it->getColor() != -1)
				continue;
			if (next == NULL) {
				next = &(*it);
				continue;
			}
			if (it->getSaturation() > next->getSaturation()) {
				next = &(*it);
				continue;
			}
			if (it->getSaturation() == next->getSaturation() && it->getDegree() > next->getDegree()) {
				next = &(*it);
				continue;
			}
		}
		if (next == NULL) // There are no uncolored nodes left.
			break; // See? I told you we would break;
		// Now let's color this node!
		int color = 0;
		// Check if this color is valid. If not, increase it until it is.
		while (!next->isValidColor(color))
			color++;
		// Color the node.
		next->setColor(color);
		// And repeat!
	}
}

// Gets the sets of indices. This is in the form of a null-terminated array of arrays of indices.
// The index arrays are terminated with -1.
// Each of the inner arrays contains vertices of the same color, and together, they partition
// the graph.
// Note that this should only be called once per coloring, as it allocates memory each time.

int **Graph::getColorSets() {
	// Determine the size of the outer array.
	int maxColor = 0;
	vector<GraphNode>::iterator it;
	for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
		int color = it->getColor();
		if (color < 0)
			return NULL; // The graph must be fully colored first!
		if (color > maxColor)
			maxColor = color;
	}
	int **colorSets = new int*[maxColor+2];
	colorSets[maxColor+1] = NULL;
	for (int currentColor = 0; currentColor <= maxColor; currentColor++) {
		// Determine the number of vertices in this set.
		int setSize = 0;
		for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
			int color = it->getColor();
			if (color == currentColor)
				setSize++;
		}
		int *colorSet = new int[setSize+1];
		colorSet[setSize] = -1;
		int ind = 0;
		// Now fill in the vertex indices.
		for (it = this->nodes.begin(); it != this->nodes.end(); it++) {
			int color = it->getColor();
			if (color == currentColor)
				colorSet[ind++] = it->getIndex();
		}
		// Finally, add this color set to the outer array.
		colorSets[currentColor] = colorSet;
	}
	// Return the final array.
	return colorSets;
}

// Creates an AABB with the specified coordinates.

AABB::AABB(double x1, double y1, double z1, double x2, double y2, double z2) {
	this->x1 = x1;
	this->y1 = y1;
	this->z1 = z1;
	this->x2 = x2;
	this->y2 = y2;
	this->z2 = z2;

}

// Returns whether the given AABBs overlap at all.

bool AABB::intersects (AABB *other) {
	if (this->x1 > other->x2) return false;
	if (this->x2 < other->x1) return false;
	if (this->y1 > other->y2) return false;
	if (this->y2 < other->y1) return false;
	if (this->z1 > other->z2) return false;
	if (this->z2 < other->z1) return false;
	return true;
}

// Builds a list of AABB's from a 2D array. The array should be null-terminated and contain several
// arrays of six values each: [x1,y1,z1,x2,y2,z2].

vector<AABB> getAABBs(double **coords) {
	vector<AABB> vec;
	for (int i = 0; coords[i] != NULL; i++) {
		double x1 = coords[i][0];
		double y1 = coords[i][1];
		double z1 = coords[i][2];
		double x2 = coords[i][3];
		double y2 = coords[i][4];
		double z2 = coords[i][5];
		AABB thisAABB(x1,y1,z1,x2,y2,z2);
		vec.push_back(thisAABB);
	}
	return vec;
}

// Builds a list of AABB's from a file. The file should contain several lines of the form
// x1 y1 z1 x2 y2 z2, with no whitespace at the end.

vector<AABB> getAABBs(char *fname) {
	vector<AABB> vec;
	ifstream file(fname);
	while (file.good()) {
		double x1, y1, z1, x2, y2, z2;
		file >> x1;
		file >> y1;
		file >> z1;
		file >> x2;
		file >> y2;
		file >> z2;
		if (!file.good()) break;
		AABB thisAABB(x1,y1,z1,x2,y2,z2);
		vec.push_back(thisAABB);
	}
	return vec;
}

// Builds the collision graph for a given vector of AABBs.

Graph getCollisions(vector<AABB> AABBs) {
	vector<AABB>::iterator it1, it2;
	int i, j;
	Graph graph;
	for (i = 0; i < AABBs.size(); i++)
		graph.addNode(i);
	i = 0;
	for (it1 = AABBs.begin(); it1 != AABBs.end(); it1++) {
		j = 0;
		for (it2 = AABBs.begin(); it2 != it1; it2++) {
			if (it1->intersects(&(*it2))) {
				graph.addEdge(i,j);
			}
			j++;
		}
		i++;
	}
	return graph;
}

// Tests the algorithm with the Petersen graph.

//void testPetersen() {
//	cout << "Testing Petersen graph" << endl;
//	Graph petersen;
//	// Add the nodes and edges.
//	for (int i=0;i<10;i++)
//		petersen.addNode(i);
//	petersen.addEdge(0,1);
//	petersen.addEdge(1,2);
//	petersen.addEdge(2,3);
//	petersen.addEdge(3,4);
//	petersen.addEdge(4,0);
//	petersen.addEdge(0,5);
//	petersen.addEdge(1,6);
//	petersen.addEdge(2,7);
//	petersen.addEdge(3,8);
//	petersen.addEdge(4,9);
//	petersen.addEdge(5,7);
//	petersen.addEdge(7,9);
//	petersen.addEdge(9,6);
//	petersen.addEdge(6,8);
//	petersen.addEdge(8,5);
//	cout << "Petersen graph created with " << petersen.getSize() << " vertices, ";
//	cout << petersen.getEdgeCount() << " edges; node 0 has degree ";
//	cout << petersen.findNode(0)->getDegree() << endl;
//	// Perform and display the coloring.
//	petersen.colorDSATUR();
//	int **colorSets = petersen.getColorSets();
//	for (int i = 0; colorSets[i] != NULL; i++) {
//		cout << "Color " << i << ": vertices ";
//		for (int j = 0; colorSets[i][j] != -1; j++) {
//			if (j > 0) cout << ", ";
//			cout << colorSets[i][j];
//		}
//		cout << endl;
//	}
//}

// Tests the algorithm with a random graph of n vertices and approximately m edges.

//void testRandom(int n, int m) {
//	cout << "Testing random large graph" << endl;
//	Graph large;
//	// Add the nodes and edges.
//	for (int i=0;i<n;i++)
//		large.addNode(i);
//	for (int i=0;i<m;i++)
//		large.addEdge(rand()%n,rand()%n);
//	cout << "Large graph created with " << large.getSize() << " vertices, ";
//	cout << large.getEdgeCount() << " edges; node 0 has degree ";
//	cout << large.findNode(0)->getDegree() << endl;
//	// Perform and display the coloring.
//	large.colorDSATUR();
//	int **colorSets = large.getColorSets();
//	for (int i = 0; colorSets[i] != NULL; i++) {
//		cout << "Color " << i << ": vertices ";
//		for (int j = 0; colorSets[i][j] != -1; j++) {
//			if (j > 0) cout << ", ";
//			cout << colorSets[i][j];
//		}
//		cout << endl;
//	}
//}

// Tests the algorithm given a graph to use.

//void testGraph(Graph *graph) {
//	cout << "Graph has " << graph->getSize() << " vertices, ";
//	cout << graph->getEdgeCount() << " edges; node 0 has degree ";
//	cout << graph->findNode(0)->getDegree() << endl;
//	// Perform and display the coloring.
//	graph->colorDSATUR();
//	int **colorSets = graph->getColorSets();
//	for (int i = 0; colorSets[i] != NULL; i++) {
//		cout << "Color " << i << ": vertices ";
//		for (int j = 0; colorSets[i][j] != -1; j++) {
//			if (j > 0) cout << ", ";
//			cout << colorSets[i][j];
//		}
//		cout << endl;
//	}
//}

//int main(int argc, char *argv[]) {
//	if (argc < 2) {
//		cout << "Please specify a filename." << endl;
//		return 0;
//	}
//	clock_t startTime = clock();
//	vector<AABB> AABBs = getAABBs(argv[1]);
//	Graph graph = getCollisions(AABBs);
//	testGraph(&graph);
//	cout << "Execution took " << double( clock() - startTime ) * 1000.0 / (double)CLOCKS_PER_SEC << " ms" << endl;
//	return 0;
//}
