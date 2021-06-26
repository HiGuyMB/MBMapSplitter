//
//  main.m
//  MBMapSplitter
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

#import <Foundation/Foundation.h>
#import "tinycthread.h"
#import "aabbcolor.h"

#include <vector>
#include <iostream>

#define FATAL(...) \
	NSLog(@"Fatal: " __VA_ARGS__); \
	return 1;

using namespace std;

static NSMutableArray *gBrushes;
static NSMutableArray *gMaps;

static NSString *gMapFile;
static NSString *gMapBase;
static NSString *gMapName;
static NSString *gMapConts;
static NSString *gMap2difPath;

static NSMutableString *gHeader;

bool map2dif(NSString *mapPath) {
	NSString *mapDir = [mapPath stringByDeletingLastPathComponent];
	NSString *difPath = [[mapDir stringByAppendingPathComponent:[mapPath.lastPathComponent stringByDeletingPathExtension]] stringByAppendingPathExtension:@"dif"];

	NSTask *map2difTask = [[NSTask alloc] init];

	[map2difTask setLaunchPath:gMap2difPath];
	[map2difTask setArguments:@[@"-t", mapDir, @"-o", mapDir, mapPath]];

	//Read stdout/stderr
	NSPipe *pipe = [NSPipe pipe];
	NSFileHandle *handle = [pipe fileHandleForReading];

	[map2difTask setStandardOutput:pipe];
	[map2difTask setStandardError:pipe];

	[map2difTask launch];

	NSMutableData *data = [NSMutableData data];

	do {
		sleep(1);
		[data appendData:[handle readDataToEndOfFile]];
	} while ([map2difTask isRunning]);

	NSString *output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
	//NSString *logPath = [NSString stringWithFormat:@"%@.log", mapPath];
	//[output writeToFile:logPath atomically:YES encoding:NSUTF8StringEncoding error:nil];

	NSLog(@"Got %ld chars of output!", data.length);

	//Check for fatal
	BOOL fatal = [output rangeOfString:@"Fatal:"].location != NSNotFound;
	BOOL status = [map2difTask terminationStatus] == 0 && !fatal;

	if (!status) {
		[[NSFileManager defaultManager] removeItemAtPath:difPath error:nil];
		//[[NSFileManager defaultManager] removeItemAtPath:logPath error:nil];
	}

	return status;
}

NSString *mapName(NSRange range) {
	return [gMapBase stringByAppendingFormat:@"/%@%ld-%ld.map", gMapName, range.location, range.length];
}

void testGraph(Graph *graph) {
	// Perform and display the coloring.
	graph->colorDSATUR();
	int **colorSets = graph->getColorSets();
	for (int i = 0; colorSets[i] != NULL; i++) {
		NSMutableString *output = [NSMutableString string];

		[output appendString:@"{\n"];
		[output appendString:gHeader];

		for (int j = 0; colorSets[i][j] != -1; j++) {
			[output appendString:gBrushes[colorSets[i][j]]];
			[output appendString:@"\n"];
		}

		[output appendString:@"\n}"];

		NSString *file = [NSString stringWithFormat:@"%@/%@-%d.map", gMapBase, gMapName, i];
		[output writeToFile:file atomically:YES encoding:NSUTF8StringEncoding error:nil];

//		if (!map2dif(file)) {
//			NSLog(@"Get pissed at map2dif.");
//		}
	}

	//Now prepare their output
	NSMutableString *output = [NSMutableString string];
	for (NSUInteger i = 0; colorSets[i] != NULL; i ++) {
		[output appendFormat:@"new InteriorInstance() {\n"
		 @"   position = \"0 0 0\";\n"
		 @"   rotation = \"1 0 0 0\";\n"
		 @"   scale = \"1 1 1\";\n"
		 @"   interiorFile = \"~/data/multiplayer/interiors/custom/%@-%d.dif\";\n"
		 @"   showTerrainInside = \"1\";\n"
		 @"};\n", gMapName, i];
	}

	[output writeToFile:[[gMapBase stringByAppendingPathComponent:gMapName] stringByAppendingPathExtension:@"cs"] atomically:YES encoding:NSUTF8StringEncoding error:nil];
}

AABB getBrushAABB(NSString *string) {
	//Find all points within ()

	NSMutableArray *verts = [NSMutableArray array];

	BOOL inVert = NO;
	NSMutableString *currentVert = [NSMutableString string];

	for (NSUInteger i = 0; i < string.length; i ++) {
		char cur = [string characterAtIndex:i];
		if (cur == '(') {
			inVert = YES;
			[currentVert deleteCharactersInRange:NSMakeRange(0, currentVert.length)];
			continue;
		}
		if (cur == ')') {
			inVert = NO;
			[verts addObject:[NSString stringWithString:currentVert]];
			continue;
		}
		if (inVert) {
			[currentVert appendFormat:@"%c", cur];
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

	for (NSUInteger i = 0; i < verts.count; i ++) {
		NSArray *comps = [[verts[i] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] componentsSeparatedByString:@" "];
		aabb[0] = MIN(aabb[0], [comps[0] doubleValue]);
		aabb[1] = MIN(aabb[1], [comps[1] doubleValue]);
		aabb[2] = MIN(aabb[2], [comps[2] doubleValue]);
		aabb[3] = MAX(aabb[3], [comps[0] doubleValue]);
		aabb[4] = MAX(aabb[4], [comps[1] doubleValue]);
		aabb[5] = MAX(aabb[5], [comps[2] doubleValue]);
	}

	return AABB(aabb[0], aabb[1], aabb[2], aabb[3], aabb[4], aabb[5]);
}

int main(int argc, const char *argv[]) {
	@autoreleasepool {
		gMap2difPath = [[NSBundle mainBundle] pathForResource:@"MBmap2difplus" ofType:@""];

		//Read your mapfile
		gMapFile = [NSString stringWithUTF8String:argv[1]];
		gMapBase = [gMapFile stringByDeletingLastPathComponent];
		gMapName = [[gMapFile lastPathComponent] stringByDeletingPathExtension];
		gMapConts = [NSString stringWithContentsOfFile:gMapFile encoding:NSUTF8StringEncoding error:nil];

		//Split it up into chunks
		NSMutableString *currentBrush = [NSMutableString string];
		gHeader = [NSMutableString string];
		gBrushes = [NSMutableArray array];

		int inGroups = 0;
		BOOL foundHeader = NO;

		vector<AABB> AABBs;

		for (NSUInteger i = 0; i < gMapConts.length; i ++) {
			char cur = [gMapConts characterAtIndex:i];
			if (cur == '{') {
				inGroups ++;

				//Worldspawn group start
				if (inGroups == 1) {
					continue;
				}

				foundHeader = YES;

				//InGroup, new brush, clear old brush
				[currentBrush deleteCharactersInRange:NSMakeRange(0, currentBrush.length)];
			}
			if (inGroups == 1 && !foundHeader) {
				[gHeader appendFormat:@"%c", cur];
			}
			if (inGroups == 2) {
				[currentBrush appendFormat:@"%c", cur];
			}
			if (cur == '}') {
				inGroups --;

				if (inGroups < 0) {
					//FATAL!
					FATAL(@"Mismatched end brace!");
				}

				//End of brush
				if (inGroups == 1) {
					[gBrushes addObject:[NSString stringWithString:currentBrush]];

					//Format brush
					AABBs.push_back(getBrushAABB(currentBrush));
				}
			}
		}

		NSLog(@"Found %ld brushes", gBrushes.count);
		Graph graph = getCollisions(AABBs);

		testGraph(&graph);
	}
    return 0;
}

