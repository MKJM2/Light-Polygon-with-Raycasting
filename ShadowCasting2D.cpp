#include <iostream>
#include <fstream>
#include <algorithm>
#include <string.h>
using namespace std;

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"


struct sEdge
{
	float sx, sy; //Start point
	float ex, ey; //End point
};

struct sCell
{
	int edge_id[4];
	bool edge_exist[4];
	bool exist = false;
};

#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3

class ShadowCasting2D : public olc::PixelGameEngine
{
public:
	ShadowCasting2D()
	{
		sAppName = "ShadowCasting2D";
	}

private:
	sCell* world;
	int nWorldWidth = 40;
	int nWorldHeight = 30;
	bool DEBUG = false;

	//Load in the light sprite
	olc::Sprite *sprLightCast;
	
	//Sprites for off-screen rendering
	olc::Sprite *buffLightRay;
	olc::Sprite *buffLightTex;


	vector<sEdge> vecEdges;
	//           angle    x     y
	vector<tuple<float, float,float>> vecVisibilityPolygonPoints;

	void ConvertTileMapToPolyMap(int sx, int sy, int w, int h, float fBlockWidth, int pitch)
	{
		//Clear PolyMap
		vecEdges.clear();

		for (int x = 0; x < w; x++)
			for (int y = 0; y < h; y++)
				for (int j = 0; j < 4; j++)
				{
					world[(y + sy) * pitch + (x + sx)].edge_exist[j] = false;
					world[(y + sy) * pitch + (x + sx)].edge_id[j] = 0;

				}

		// Iterate through region from top left to bottom right
		for (int x = 1; x < w - 1; x++)
			for (int y = 1; y < h - 1; y++)
			{
				// Create some convenient indices
				int i = (y + sy) * pitch + (x + sx); 		//This
				int n = (y + sy - 1) * pitch + (x + sx);	//Northern neighbour
				int s = (y + sy + 1) * pitch + (x + sx);	//Southern neighbour
				int w = (y + sy) * pitch + (x + sx - 1); 	//Western neighbour
				int e = (y + sy) * pitch + (x + sx + 1);	//Eastern neighbour 
				//Alternatively, e = w + 2?

				// If this cell exists, check if it needs edges
				if (world[i].exist)
				{
					
					// If this cell has no western neighbour, it needs a western edge
					if(!world[w].exist)
					{
						//It can either extend it from its norther neighbour
						// or it can start a new one.
						if (world[n].edge_exist[WEST])
						{
							// Northern neighbour has a western edge, so grow it downwards
							vecEdges[world[n].edge_id[WEST]].ey += fBlockWidth;
							world[i].edge_id[WEST] = world[n].edge_id[WEST];
							world[i].edge_exist[WEST] = true;
						} 
						else
						{
							// Northern neighbour does not have one, so create one
							sEdge edge;
							edge.sx = (sx + x) * fBlockWidth;
							edge.sy = (sy + y) * fBlockWidth;
							edge.ex = edge.sx; edge.ey = edge.sy + fBlockWidth;

							int edge_id = vecEdges.size();
							vecEdges.push_back(edge);

							world[i].edge_id[WEST] = edge_id;
							world[i].edge_exist[WEST] = true;
						}
					}
					// If this cell dont have an eastern neignbour, It needs a eastern edge
					if (!world[e].exist)
					{
						// It can either extend it from its northern neighbour if they have
						// one, or It can start a new one.
						if (world[n].edge_exist[EAST])
						{
							// Northern neighbour has one, so grow it downwards
							vecEdges[world[n].edge_id[EAST]].ey += fBlockWidth;
							world[i].edge_id[EAST] = world[n].edge_id[EAST];
							world[i].edge_exist[EAST] = true;
						}
						else
						{
							// Northern neighbour does not have one, so create one
							sEdge edge;
							edge.sx = (sx + x + 1) * fBlockWidth; edge.sy = (sy + y) * fBlockWidth;
							edge.ex = edge.sx; edge.ey = edge.sy + fBlockWidth;

							// Add edge to Polygon Pool
							int edge_id = vecEdges.size();
							vecEdges.push_back(edge);

							// Update tile information with edge information
							world[i].edge_id[EAST] = edge_id;
							world[i].edge_exist[EAST] = true;
						}
					}

					// If this cell doesnt have a northern neignbour, It needs a northern edge
					if (!world[n].exist)
					{
						// It can either extend it from its western neighbour if they have
						// one, or It can start a new one.
						if (world[w].edge_exist[NORTH])
						{
							// Western neighbour has one, so grow it eastwards
							vecEdges[world[w].edge_id[NORTH]].ex += fBlockWidth;
							world[i].edge_id[NORTH] = world[w].edge_id[NORTH];
							world[i].edge_exist[NORTH] = true;
						}
						else
						{
							// Western neighbour does not have one, so create one
							sEdge edge;
							edge.sx = (sx + x) * fBlockWidth; edge.sy = (sy + y) * fBlockWidth;
							edge.ex = edge.sx + fBlockWidth; edge.ey = edge.sy;

							// Add edge to Polygon Pool
							int edge_id = vecEdges.size();
							vecEdges.push_back(edge);

							// Update tile information with edge information
							world[i].edge_id[NORTH] = edge_id;
							world[i].edge_exist[NORTH] = true;
						}
					}

					// If this cell doesnt have a southern neignbour, It needs a southern edge
					if (!world[s].exist)
					{
						// It can either extend it from its western neighbour if they have
						// one, or It can start a new one.
						if (world[w].edge_exist[SOUTH])
						{
							// Western neighbour has one, so grow it eastwards
							vecEdges[world[w].edge_id[SOUTH]].ex += fBlockWidth;
							world[i].edge_id[SOUTH] = world[w].edge_id[SOUTH];
							world[i].edge_exist[SOUTH] = true;
						}
						else
						{
							// Western neighbour does not have one, so I need to create one
							sEdge edge;
							edge.sx = (sx + x) * fBlockWidth; edge.sy = (sy + y + 1) * fBlockWidth;
							edge.ex = edge.sx + fBlockWidth; edge.ey = edge.sy;

							// Add edge to Polygon Pool
							int edge_id = vecEdges.size();
							vecEdges.push_back(edge);

							// Update tile information with edge information
							world[i].edge_id[SOUTH] = edge_id;
							world[i].edge_exist[SOUTH] = true;
						}
					}
				}
			}
	}

	void CalculateVisibilityPolygon(float ox, float oy, float radius)
	{
		// Get rid of existing polygon
		vecVisibilityPolygonPoints.clear();

		// For each edge in PolyMap
		for (auto &e1 : vecEdges)
		{
			// Take the start points, then the end points
			for (int i = 0; i < 2; i++)
			{

				//Ray direction as a (x,y) vector
				float rdx, rdy;
				rdx = (i == 0 ? e1.sx : e1.ex) - ox;
				rdy = (i == 0 ? e1.sy : e1.ey) - oy;
				
				float base_ang = atan2f(rdy, rdx);
				float ang = 0;


				//Cast the 3 rays to the endpoint
				for (int j = 0; j < 3; j++)
				{
					if (j == 0) ang = base_ang - 0.0001f;
					if (j == 1) ang = base_ang;
					if (j == 2) ang = base_ang + 0.0001f;

					rdx = radius * cosf(ang);
					rdy = radius * sinf(ang);

					float min_t1 = INFINITY;
					float min_px = 0, min_py = 0, min_ang = 0;
					bool bValid = false;

					for (auto &e2 : vecEdges)
					{
						// Create line segment vector
						float sdx = e2.ex - e2.sx;
						float sdy = e2.ey - e2.sy;
						
						//Check if not linear
						if (fabs(sdx - rdx) > 0.0f && fabs(sdy - rdy) > 0.0f)
						{
							// t2 is normalised distance from line segment start to line segment end of intersect point
							float t2 = (rdx * (e2.sy - oy) + (rdy * (ox - e2.sx))) / (sdx * rdy - sdy * rdx);
							// t1 is normalised distance from source along ray to ray length of intersect point

							float t1 = (0.0001f < rdx && rdx < 0.0001f) ? (e2.sx + sdx * t2 - ox) / rdx : (e2.sy + sdy * t2 - oy) / rdy;

							// If intersect point exists along ray, and along line 
							// segment then intersect point is valid
							if (t1 > 0 && t2 >= 0 && t2 <= 1.0f)
							{
								// Check if this intersect point is closest to source. If
								// it is, then store this point and reject others
								if (t1 < min_t1)
								{
									min_t1 = t1;
									min_px = ox + rdx * t1;
									min_py = oy + rdy * t1;
									min_ang = atan2f(min_py - oy, min_px - ox);
									bValid = true;
								}
							}
						}
					}
					if(bValid)// Add intersection point to visibility polygon perimeter
						vecVisibilityPolygonPoints.push_back({ min_ang, min_px, min_py });
				}
			}
		}


		// Sort perimeter points by angle from source
		// This allows us to draw the triangle fan
		 sort(
			vecVisibilityPolygonPoints.begin(),
			vecVisibilityPolygonPoints.end(),
			[&](const tuple<float, float, float> &t1, const tuple<float, float, float> &t2)
			{
				return get<0>(t1) < get<0>(t2);
			});
	}
public: 
	//Save the tilemap to be read during the next run of the program
	bool SaveMapData(string fileName = "data.bin"){
		ofstream f (fileName);
		if (f.is_open())
		{
			for (int i = 0; i < nWorldWidth * nWorldHeight; i++)
			{
					f << world[i].exist;
			}
			f.close();
			return true;
		} else return false;
	}

	//Read tilemap from disk
	bool ReadMapData(string fileName = "data.bin"){
		ifstream f (fileName);
		if (f.is_open())
		{
			char byte;
			bool b;
			int i = 0;
			while ( f.get(byte) && i < (nWorldHeight * nWorldWidth))
			{
				b = (byte == '1');
				world[i].exist = b;
				i++;
			}
			f.close();
			return true;
		} else return false;
	}


public:
	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		world = new sCell[nWorldWidth * nWorldHeight];

		if (!ReadMapData())
		{
			/* Add a boundary to the world */
			for (int x = 1; x < (nWorldWidth - 1); x++)
			{
				world[1 * nWorldWidth + x].exist = true;
				world[(nWorldHeight - 2) * nWorldWidth + x].exist = true;
			}

			for (int x = 1; x < (nWorldHeight - 1); x++)
			{
				world[x * nWorldWidth + 1].exist = true;
				world[x * nWorldWidth + (nWorldWidth - 2)].exist = true;
			}
		}
		
		//Initialize sprite and image buffers
		sprLightCast = new olc::Sprite("light_cast.png");
		buffLightTex = new olc::Sprite(ScreenWidth(), ScreenHeight());
		buffLightRay = new olc::Sprite(ScreenWidth(), ScreenHeight());

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		float fBlockWidth = 16.0f;
		float fSourceX = GetMouseX();
		float fSourceY = GetMouseY();

		if (GetMouse(0).bReleased)
		{	// i = y * width + x
			int i = ((int)fSourceY / (int)fBlockWidth) * nWorldWidth + ((int)fSourceX / (int)fBlockWidth);
			world[i].exist = !world[i].exist;

			// Take a region of "Tilemap" and convert it to "PolyMap"
			ConvertTileMapToPolyMap(0, 0, 40, 30, fBlockWidth, nWorldWidth);
		}

		if (GetKey(olc::SPACE).bReleased) DEBUG = !DEBUG;

		if (GetMouse(1).bHeld)
		{
			CalculateVisibilityPolygon(fSourceX, fSourceY, 1000.0f);
		}


		//Drawing
		SetDrawTarget(nullptr); //Draw to screen
		Clear(olc::BLACK);


		int nRaysCast = vecVisibilityPolygonPoints.size();

		// Remove duplicate (or simply similar) points from polygon
		auto it = unique(
			vecVisibilityPolygonPoints.begin(),
			vecVisibilityPolygonPoints.end(),
			[&](const tuple<float, float, float> &t1, const tuple<float, float, float> &t2)
			{
				return fabs(get<1>(t1) - get<1>(t2)) < 0.1f && fabs(get<2>(t1) - get<2>(t2)) < 0.1f;
			});

		vecVisibilityPolygonPoints.resize(distance(vecVisibilityPolygonPoints.begin(), it));

		int nRaysCast2 = vecVisibilityPolygonPoints.size();
		DrawString(4, 4, "Rays Cast: " + to_string(nRaysCast) + " Rays Drawn: " + to_string(nRaysCast2));

		// If drawing rays, set an offscreen texture as our target buffer
		if (GetMouse(1).bHeld && vecVisibilityPolygonPoints.size() > 1)
		{

			// Clear offscreen buffer for sprite
			SetDrawTarget(buffLightTex);
			Clear(olc::BLACK);

			// Draw the light sprite to offscreen buffer, centered around
			// light source location (mouse coords), buffer is 512x512 (sprite texture res)
			DrawSprite(fSourceX - 255, fSourceY - 255, sprLightCast);

			//Clear offscreenbuffer for light rays
			SetDrawTarget(buffLightRay);
			Clear(olc::BLANK);

			
			if (DEBUG) {
				SetDrawTarget(nullptr);

				//Draw rays
				for (int i = 0; i < vecVisibilityPolygonPoints.size() - 1; i++)
				{
						DrawTriangle(
							fSourceX,
							fSourceY,

							get<1>(vecVisibilityPolygonPoints[i]),
							get<2>(vecVisibilityPolygonPoints[i]),

							get<1>(vecVisibilityPolygonPoints[i + 1]),
							get<2>(vecVisibilityPolygonPoints[i + 1]),
							olc::RED);

				}
				

				//Draw in the last triangle (from starting point to endpoint)
				DrawTriangle(
					fSourceX,
					fSourceY,

					get<1>(vecVisibilityPolygonPoints[vecVisibilityPolygonPoints.size() - 1]),
					get<2>(vecVisibilityPolygonPoints[vecVisibilityPolygonPoints.size() - 1]),

					get<1>(vecVisibilityPolygonPoints[0]),
					get<2>(vecVisibilityPolygonPoints[0]),
					olc::RED);
			}
			else
			{
				//Draw each triangle in fan
				for (int i = 0; i < vecVisibilityPolygonPoints.size() - 1; i++)
				{
						FillTriangle(
							fSourceX,
							fSourceY,

							get<1>(vecVisibilityPolygonPoints[i]),
							get<2>(vecVisibilityPolygonPoints[i]),

							get<1>(vecVisibilityPolygonPoints[i + 1]),
							get<2>(vecVisibilityPolygonPoints[i + 1]));

				}
				

				//Draw in the last triangle (from starting point to endpoint)
				FillTriangle(
					fSourceX,
					fSourceY,

					get<1>(vecVisibilityPolygonPoints[vecVisibilityPolygonPoints.size() - 1]),
					get<2>(vecVisibilityPolygonPoints[vecVisibilityPolygonPoints.size() - 1]),

					get<1>(vecVisibilityPolygonPoints[0]),
					get<2>(vecVisibilityPolygonPoints[0]));
			}

			//Wherever rays exist in ray sprite, copy over radial light sprite
			SetDrawTarget(nullptr);
			for (int x = 0; x < ScreenWidth(); x++)
				for (int y = 0; y < ScreenHeight(); y++)
				{
					if (buffLightRay->GetPixel(x, y).r > 0)
						Draw(x, y, buffLightTex->GetPixel(x,y));
				}

		}
		
		// Draw Blocks from TileMap
		for (int x = 0; x < nWorldWidth; x++)
			for (int y = 0; y < nWorldHeight; y++)
			{
				if (world[y * nWorldWidth + x].exist)
					FillRect(x * fBlockWidth, y * fBlockWidth, fBlockWidth, fBlockWidth, (DEBUG == false) ? olc::BLUE : olc::BLANK);
			}
		
		if (DEBUG)
		{
			for (auto &e : vecEdges)
			{
				DrawLine(e.sx, e.sy, e.ex, e.ey);
				FillCircle(e.sx, e.sy, 1, olc::RED);
				FillCircle(e.ex, e.ey, 1, olc::RED);
			}
		}

		return true;
	}
};


int main()
{
	ShadowCasting2D demo;
	if (demo.Construct(640, 480, 2, 2))
		demo.Start();
		//Deinitialization
		demo.SaveMapData();


	
	return 0;
}

