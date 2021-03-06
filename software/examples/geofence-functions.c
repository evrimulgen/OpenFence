/*
 * Examples of how to create a simple Geofence.
 * Copyright (C) 2016,  Alex Muir 
 * https://hackaday.io/project/10725-openfence-digital-livestock-fencing
 * https://hackaday.io/project/10725-openfence-digital-livestock-fencing/log/35952-geofence-functionality
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 // Build this program using "gcc geofence-functions.c -o geofence-functions"
 // Then run using "./geofence-functions"


#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#define polyCorners 4  //Define the number of corners the polygon has

//Macros
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

//Struct to hold the lat/lon for each point in degrees and radians
struct position{
  double lat; //x
  double lon; //y
  double latrad; //x
  double lonrad; //y
};

struct position points[polyCorners];
struct position me;

double degrees2radians(double degrees){
    return degrees*M_PI/180;
}
double radians2degrees(double radians){
    return radians*180/M_PI;
}
double sqr(double x){
  return x*x;
}

//Step 1: Different Distance Calculation Functions
double distanceEquir(struct position v, struct position w) {   //Equirectangular Approximation
  double p1 = (v.lonrad - w.lonrad)* cos( 0.5*(v.latrad+w.latrad) ) ;
  double p2 = (v.latrad - w.latrad);
  return 6371000 * sqrt( p1*p1 + p2*p2);
}  
double distanceSpher(struct position v, struct position w) {   //Spherical Law of Cosines
  return acos( sin(v.latrad)*sin(w.latrad) + cos(v.latrad)*cos(w.latrad)*cos(w.lonrad-v.lonrad) ) * 6371000;
}   
double distanceHaver(struct position v, struct position w){   //Haversine 
  double a = sin((w.latrad-v.latrad)/2) * sin((w.latrad-v.latrad)/2)+
        cos(v.latrad) * cos(w.latrad) *
        sin((w.lonrad-v.lonrad)/2) * sin((w.lonrad-v.lonrad)/2);
  double dist = 6371000 * 2 * atan2(sqrt(a), sqrt(1-a)); //Metres
  return dist;
}

//Step 2: Test if the point is within the polygon of points
bool pointInPolygon() {
  //Based on http://alienryderflex.com/polygon/
  //oddNodes = 1 means within the polygon, oddNodes = 0 outside the polygon.
  int   i, j=polyCorners-1 ;
  bool  oddNodes=0;

  for(i=0; i<polyCorners; i++) {
    if(((points[i].latrad< me.latrad && points[j].latrad>=me.latrad) 
      || (points[j].latrad< me.latrad && points[i].latrad>=me.latrad))  
      &&  (points[i].lonrad<=me.lonrad || points[j].lonrad<=me.lonrad)) {
      oddNodes^=(points[i].lonrad+(me.latrad-points[i].latrad) / 
        (points[j].latrad-points[i].latrad)*(points[j].lonrad-points[i].lonrad)<me.lonrad); 
    }
    j=i; 
  }
  return oddNodes; 
}

//Step 3: Find which sides of the boundary we are outside
double distBehind(struct position p, struct position w, struct position v){
  //Returns a negative if outside that boundary.
  double Fplat =w.lat;
  double Fplon =w.lon;
  //Calculate the unit length normal vector: Fn
  double Fnlat =  w.lon-v.lon;       // y' 
  double Fnlon = - (w.lat-v.lat);    //-x'
  double mag=sqrt(sqr(Fnlat)+sqr(Fnlon));
  Fnlat /= mag;           //Unit length
  Fnlon /= mag;
  //p-Fp
  Fplat = p.lat- Fplat;   //Reuse variables
  Fplon = p.lon - Fplon;
  //Return the dot product
  return Fplat*Fnlat + Fplon*Fnlon;
}

// //Step 4: Get an accurate shortest distance to a side of the fence
// double dist2segment(struct position p, struct position v, struct position w){
//   //Check if the two side points are in the same location (avoid dividing by zero later)
//   double l = distanceEquir(v,w);
//   if(l==0) return distanceEquir(p,v);
//   //Find the max and min x and y points
//   double minx = MIN(v.lat, w.lat);
//   double maxx = MAX(v.lat, w.lat);
//   double miny = MIN(v.lon, w.lon);
//   double maxy = MAX(v.lon, w.lon);

//   struct position projection;
//   if(p.lat<minx && p.lon<miny){         //p does not fall between the two points and is closest to the lower corner
//     projection.latrad = degrees2radians(minx); 
//     projection.lonrad = degrees2radians(miny); 
//   }else if(p.lat>maxx && p.lon>maxy){   //p does not fall between the two points and is closest to the lower corner
//     projection.latrad = degrees2radians(maxx); 
//     projection.lonrad = degrees2radians(maxy); 
//   }else{                                //p does fall between the two points, project point onto the line
//     double x1=v.lat, y1=v.lon, x2=w.lat, y2=w.lon, x3=p.lat, y3=p.lon;
//     double px = x2-x1, py = y2-y1, dAB = px*px + py*py;
//     double u = ((x3 - x1) * px + (y3 - y1) * py) / dAB;
//     double x = x1 + u * px, y = y1 + u * py;
//     projection.latrad = degrees2radians(x); 
//     projection.lonrad = degrees2radians(y); 
//   }
//   //Return the distance to the closest point on the line.
//   return distanceEquir(p, projection);
// }

//Step 4: Get an accurate shortest distance to a side of the fence
float dist2segment(struct position p, struct position v, struct position w, struct position * projection){
  //Check if the two side points are in the same location (avoid dividing by zero later)
  float l = distanceEquir(v,w);
  if(l==0) return distanceEquir(p,v);
  //Find the max and min x and y points
  float minx = MIN(v.latrad, w.latrad);
  float maxx = MAX(v.latrad, w.latrad);
  float miny = MIN(v.lonrad, w.lonrad);
  float maxy = MAX(v.lonrad, w.lonrad);

  //struct position projection;
  if(p.latrad<minx && p.lonrad<miny){         //p does not fall between the two points and is closest to the lower corner
    projection->latrad = minx; 
    projection->lonrad = miny; 
  }else if(p.latrad>maxx && p.lonrad>maxy){   //p does not fall between the two points and is closest to the lower corner
    projection->latrad = maxx; 
    projection->lonrad = maxy; 
  }else{                                //p does fall between the two points, project point onto the line
    float x1=v.latrad, y1=v.lonrad, x2=w.latrad, y2=w.lonrad, x3=p.latrad, y3=p.lonrad;
    float px = x2-x1, py = y2-y1, dAB = px*px + py*py;
    float u = ((x3 - x1) * px + (y3 - y1) * py) / dAB;
    float x = x1 + u * px, y = y1 + u * py;
    projection->latrad = x; 
    projection->lonrad = y; 
  }
  //Return the distance to the closest point on the line.
  return distanceEquir(p, *projection);
}


float bearing2fence(struct position p, struct position projection){
  float y = sin(p.lonrad-projection.lonrad) * cos(p.latrad);
  float x = cos(projection.latrad)*sin(p.latrad) - sin(projection.latrad)*cos(p.latrad)*cos(p.lonrad-projection.lonrad);
  float brng = atan2(y, x);
  if (brng>0)
      {
        brng=brng-2*M_PI;
      }
      brng=2*M_PI+brng;

  return brng;
}


int main()
{
  //Some Test Points
	points[0].lat = -37.911318; 
	points[0].lon = 145.138143;
	points[1].lat = -37.911462;
	points[1].lon = 145.139120;
	points[2].lat = -37.912300;
	points[2].lon = 145.138902;
	points[3].lat = -37.912135;
	points[3].lon = 145.137907;

	
  //Convert degrees to radians and store in the struct
  for(int i=0; i<polyCorners; i++){
      points[i].latrad = degrees2radians(points[i].lat);
      points[i].lonrad = degrees2radians(points[i].lon);
  }

  //Three possible test positons.
	//me.lat = -37.911779; //Inside
	//me.lon = 145.138567;
	me.lat = -37.911643; //Just Outside 
  me.lon = 145.137766;
  //me.lat = -37.913647; //Well outside
  //me.lon = 145.137045;
  me.latrad = degrees2radians(me.lat);
  me.lonrad = degrees2radians(me.lon);

  // //Step 1: Compare the three distance calculations.
  // printf("\nEquir \t %.10G \n",distanceEquir(points[1],points[2]));
  // printf("Cosine \t %.10G \n",distanceSpher(points[1],points[2]));
  // printf("Haver \t %.10G \n",distanceHaver(points[1],points[2]));

  // //Step 2: Test if the point is within the boundary
  // printf("\nThe point is within the boundary: %s \n\n", pointInPolygon() ? "true" : "false");

  // //Step 3 and 4: Which sides are we outside and by how far?
  // printf("Side \t Step 3 \t Step 4 \tSide length \n");
  // printf("3<->0 \t %G \t %G \t %G\n", distBehind(me,points[polyCorners-1],points[0])*100000, dist2segment(me,points[polyCorners-1],points[0]), distanceEquir(points[polyCorners-1],points[0]));
  // for(int i=1; i<polyCorners; i++){ 
  //   printf("%i<->%i \t %G \t %G \t %G\n",i-1,i, distBehind(me,points[i-1],points[i])*100000, dist2segment(me,points[i-1],points[i]), distanceEquir(points[i-1],points[i]));
  // }

  struct position projection;

  printf("%G \t",dist2segment(me,points[polyCorners-1],points[0],&projection));
  printf("%G \n",bearing2fence(me, projection)*180/M_PI);
  printf("%G \t",dist2segment(me,points[0],points[1],&projection));
  printf("%G \n",bearing2fence(me, projection)*180/M_PI);
  printf("%G \t",dist2segment(me,points[1],points[2],&projection));
  printf("%G \n",bearing2fence(me, projection)*180/M_PI);
  printf("%G \t",dist2segment(me,points[2],points[3],&projection));
  printf("%G \n",bearing2fence(me, projection)*180/M_PI);
  printf("\n");
}