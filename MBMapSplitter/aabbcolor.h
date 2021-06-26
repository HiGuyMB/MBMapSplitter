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

// The GraphNode class. Used for the individual nodes in the collision graph.

#include <vector>

using namespace std;

class GraphNode {
private:
	int index;
	int color;
	vector<GraphNode*> neighbors;
	void vectorRemove(GraphNode *node);
public:
	GraphNode(int index);
	int getIndex();
	void setIndex(int index);
	int getColor();
	void setColor(int color);
	int addNeighbor(GraphNode *neighbor);
	int removeNeighbor(GraphNode *neighbor);
	bool isNeighbor(GraphNode *neighbor);
	int getDegree();
	int getSaturation();
	bool isValidColor(int color);
};

// The Graph class. Used to represent a graph of which AABBs collide, which is then colored.

class Graph {
private:
	vector<GraphNode> nodes;
public:
	Graph();
	int addNode(int index);
	int removeNode(int index);
	int removeNode(GraphNode *node);
	bool containsNode(int index);
	GraphNode *findNode(int index);
	int getSize();
	void addEdge(int index1, int index2);
	void removeEdge(int index1, int index2);
	bool isEdge(int index1, int index2);
	int getEdgeCount();
	void colorDSATUR();
	int **getColorSets();
};

// The AABB class provides a bit of a wrapper for the AABBs themselves. Nothing fancy.

class AABB {
private:
	double x1, y1, z1, x2, y2, z2;
public:
	AABB(double x1, double y1, double z1, double x2, double y2, double z2);
	bool intersects(AABB *other);
};

Graph getCollisions(vector<AABB> AABBs);
vector<AABB> getAABBs(char *fname);
vector<AABB> getAABBs(double **coords);
