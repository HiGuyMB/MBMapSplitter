//
//  Document.m
//  MBMapSplitterOSX
//
//  Copyright (c) 2015 HiGuy Smith
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

extern "C" {
#include "Document.h"
}

#include <stdio.h>
#include "aabbcolor.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

AABB getBrushAABB(const std::string input) {
	//Find all points within ()

	std::vector<std::string> verts;

	bool inVert = false;
	std::string currentVert;

	for (int i = 0; i < input.length(); i ++) {
		char cur = input[i];
		if (cur == '(') {
			inVert = true;
			currentVert.clear();
			continue;
		}
		if (cur == ')') {
			inVert = false;
			verts.push_back(currentVert);
			continue;
		}
		if (inVert) {
			currentVert += cur;
		}
	}

	//Now that we have the verts, AABB em
	double aabb[6];
	aabb[0] = INT_MAX;
	aabb[1] = INT_MAX;
	aabb[2] = INT_MAX;
	aabb[3] = INT_MIN;
	aabb[4] = INT_MIN;
	aabb[5] = INT_MIN;

	for (int i = 0; i < verts.size(); i ++) {
		std::string vert = verts[i];

		std::string buf;
		std::stringstream stream(vert);
		std::vector<std::string> tokens;

		while (stream >> buf) {
			tokens.push_back(buf);
		}

		aabb[0] = MIN(aabb[0], atof(tokens[0].c_str()));
		aabb[1] = MIN(aabb[1], atof(tokens[1].c_str()));
		aabb[2] = MIN(aabb[2], atof(tokens[2].c_str()));
		aabb[3] = MAX(aabb[3], atof(tokens[0].c_str()));
		aabb[4] = MAX(aabb[4], atof(tokens[1].c_str()));
		aabb[5] = MAX(aabb[5], atof(tokens[2].c_str()));
	}

	return AABB(aabb[0], aabb[1], aabb[2], aabb[3], aabb[4], aabb[5]);
}

std::string readFile(const char *path) {
	std::string conts;
	std::ifstream stream;
	stream.open(path);

	if (!stream.is_open()) {
		return std::string();
	}
	std::string line;
	while (getline(stream, line, '\n')) {
		conts.append(line);
		conts.append("\n");
	}
	stream.close();

	return conts;
}

void convertPath(std::string &path) {
	std::replace(path.begin(), path.end(), '\\', '/');
}

std::string stripPath(const std::string &path) {
#ifdef _WIN32
	size_t pos = path.find_last_of("\\");
#else
	size_t pos = path.find_last_of("/");
#endif
	if (pos == std::string::npos) {
		return path;
	}

	return path.substr(pos + 1);
}

std::string stripExt(const std::string &path) {
	size_t pos = path.find_last_of(".");
	if (pos == std::string::npos) {
		return path;
	}

	return path.substr(0, pos);
}

void printUsage(const char *executable) {
	std::cout << "Usage: " << executable << " <map file> [-e export file [-p prefix]]" << std::endl;
}

void writeCstring(std::ostream &stream, const char *string) {
	stream.write(string, strlen(string));
}

@interface Document ()

@end

static BOOL gOneShot = NO, gSetOneShot = NO;

@implementation Document

- (instancetype)init {
    self = [super init];
    if (self) {
		// Add your subclass-specific initialization here.
    }
    return self;
}

- (void)windowControllerDidLoadNib:(NSWindowController *)aController {
	[super windowControllerDidLoadNib:aController];
	if (!gSetOneShot) {
		gOneShot = ([self fileURL] != nil);
		gSetOneShot = YES;
	}

	[self performSelector:@selector(run:) withObject:[self fileURL] afterDelay:0];
}

+ (BOOL)autosavesInPlace {
	return YES;
}

- (NSString *)windowNibName {
	return @"Document";
}

- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError {
	return nil;
}

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError *__autoreleasing *)outError {
	return YES;
}

- (void)run:(NSURL *)url {
	if (url && ![self _run:url]) {
		[[NSAlert alertWithMessageText:@"Error" defaultButton:@"Close" alternateButton:nil otherButton:nil informativeTextWithFormat:@"There was an error splitting the map. Make sure that the map is readable and the map's directory is writeable."] runModal];
	}
	[self close];
	if (gOneShot) {
		[NSApp terminate:self];
	}
}

- (BOOL)_run:(NSURL *)url {
	const char *mapfile = [[url path] UTF8String];

	//Read the map
	std::string mapConts = readFile(mapfile);
	if (mapConts.length() == 0) {
		std::cout << "Invalid input file " << mapfile << std::endl;
		return NO;
	}

	//Split it into pieces
	std::string header;
	std::string currentBrush;
	std::vector<AABB> AABBs;
	std::vector<std::string> brushes;

	int inGroups = 0;
	bool foundHeader = false;

	for (int i = 0; i < mapConts.length(); i ++) {
		char cur = mapConts[i];
		if (cur == '{') {
			inGroups ++;

			//Worldspawn group start
			if (inGroups == 1) {
				continue;
			}

			foundHeader = true;

			currentBrush.clear();
		}
		if (inGroups == 1 && !foundHeader) {
			header += cur;
		}
		if (inGroups == 2) {
			currentBrush += cur;
		}
		if (cur == '}') {
			inGroups --;
			if (inGroups < 0) {
				std::cout << "Mismatched end brace in " << mapfile << std::endl;
				return NO;
			}
			//End of brush
			if (inGroups == 1) {
				brushes.push_back(currentBrush);
				AABBs.push_back(getBrushAABB(currentBrush));
			}
		}
	}

	std::cout << "Found " << brushes.size() << " brushes." << std::endl;

	//Split algorithm by Whirligig231
	Graph graph = getCollisions(AABBs);
	graph.colorDSATUR();

	//Export sets
	int **colorSets = graph.getColorSets();
	for (int i = 0; colorSets[i] != NULL; i ++) {
		// path/to/mapname-0.map
		std::string path(mapfile);
		path = stripExt(path);
		path += "-";
		path += std::to_string(i); //C++11
		path += ".map";

		//Write map
		std::ofstream output;
		output.open(path);
		if (!output.is_open()) {
			std::cout << "Could not write split map, error with " << path << std::endl;
			return NO;
		}

		output.put('{');
		writeCstring(output, header.c_str());

		//Write each brush from the set
		for (int j = 0; colorSets[i][j] != -1; j ++) {
			writeCstring(output, brushes[colorSets[i][j]].c_str());
			writeCstring(output, "\r\n");
		}

		output.put('}');
		output.close();
	}

	const char *cspath = [[url URLByDeletingPathExtension] URLByAppendingPathExtension:@"cs"].path.UTF8String;

	//Export their split map to a cs file
	std::ofstream output;
	output.open(cspath);
	if (!output.is_open()) {
		std::cout << "Could not open exports file " << cspath << std::endl;
		return NO;
	}

	//Mapname
	std::string path;
	path = stripExt(stripPath(mapfile));

	convertPath(path);

	for (int i = 0; colorSets[i] != NULL; i ++) {
		writeCstring(output, "   new InteriorInstance() {\n"
					 "      position = \"0 0 0\";\n"
					 "      rotation = \"1 0 0 0\";\n"
					 "      scale = \"1 1 1\";\n");

		//   interiorFile = "<path/to/>Mapname-0.dif";
		writeCstring(output, "      interiorFile = \"");
		writeCstring(output, path.c_str());
		writeCstring(output, "-");
		writeCstring(output, std::to_string(i).c_str());
		writeCstring(output, ".dif\";\n");
		writeCstring(output, "      showTerrainInside = \"1\";\n");
		writeCstring(output, "   };\n");
	}
	output.close();

	return YES;
}

@end
