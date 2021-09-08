#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <limits.h>
#include "libbmp.h"

typedef unsigned char byte;

// object size
#define GRID_SIZE 16
#define LAYERS 16

// multiplier for accuracy of calculation
#define SIZE_MUL 16.0
#define VIEW_SIZE 64
#define GRID_SIZE_EXT (GRID_SIZE * SIZE_MUL)
#define Q_ANGLES 8
#define ALL_ANGLES (Q_ANGLES*4)
#define HEAD_FRAMES (4 + 1 + 4)
#define HAND_FRAMES (2) /* moving up + waving*/
#define ALL_FRAMES ( ALL_ANGLES + HEAD_FRAMES + HAND_FRAMES)

#define GRID_EMPTY 1

////////////////////////////////////////////
/// precalc data to be used in intro
double view_point_x[ALL_ANGLES][VIEW_SIZE];
double view_point_y[ALL_ANGLES][VIEW_SIZE];
double ray_step_x[ALL_ANGLES];
double ray_step_y[ALL_ANGLES];
////////////////////////////////////////////

// object data
unsigned char object[LAYERS+1][GRID_SIZE][GRID_SIZE];  // +1 because we have some buffer overflow but I don't have time to check it now.

#define PAGE_SIZE 256
byte anim[ALL_FRAMES][PAGE_SIZE];

// starting position of ray
byte from_x, from_y;


double d2r(double deg)
{
	return deg * M_PI / 180.0;
}

inline double Det(double a, double b, double c, double d)
{
	return a * d - b * c;
}


typedef struct point {
	double x;
	double y;
} point;

struct point calc_intersection(struct point s1, struct point e1, struct point s2, struct point e2)
{
	double m1, c1, m2, c2;
	double x1, y1, x2, y2;
	double dx, dy;
	double intersection_X, intersection_Y;
	struct point result;

	x1 = s1.x;
	y1 = s1.y;
	x2 = e1.x;
	y2 = e1.y;

	dx = x2 - x1;
	dy = y2 - y1;
	m1 = dy / dx;

	// y = mx + c
	// intercept c = y - mx
	c1 = y1 - m1 * x1; // which is same as y2 - slope * x2


	x1 = s2.x;
	y1 = s2.y;
	x2 = e2.x;
	y2 = e2.y;

	dx = x2 - x1;
	dy = y2 - y1;
	m2 = dy / dx;

	// y = mx + c
	// intercept c = y - mx

	c2 = y1 - m2 * x1; // which is same as y2 - slope * x2

	//printf("Equation of line1: Y = %.2fX %c %.2f\n", m1, (c1 < 0) ? ' ' : '+', c1);
	//printf("Equation of line2: Y = %.2fX %c %.2f\n", m2, (c2 < 0) ? ' ' : '+', c2);

	if ((m1 - m2) == 0)
	{

		//printf("No Intersection between the lines\n");
		result.x = NAN;
		result.y = NAN;
		return result;
	}
	else
	{
		// if 
		if (x2 == x1)
		{
			intersection_X = x1;
			intersection_Y = m1 * x1 + c1;
		}
		else
		{
			intersection_X = (c2 - c1) / (m1 - m2);
			intersection_Y = m1 * intersection_X + c1;
		}

		//printf("Intersecting Point: = %.2f, %.2f\n", intersection_X, intersection_Y);

		result.x = intersection_X;
		result.y = intersection_Y;
		return result;
	}
}

double distance(point start, point end)
{
	double dx = end.x - start.x;
	double dy = end.y - start.y;
	return sqrt(dx * dx + dy * dy);
}

struct point point_from_angle(struct point start, double radius, double angle)
{
	struct point result;
	result.x = start.x + radius * cos(d2r(270+angle));
	result.y = start.y + radius * sin(d2r(270 + angle));
	return result;
}

void save_to_file(char* filename, byte* data, unsigned int size)
{
	FILE* fp = fopen(filename, "wb+");
	fwrite(data, 1, size, fp);
	fclose(fp);
}

void save_data()
{
	save_to_file("anim.dat", (byte*)anim, sizeof(anim));
}

void precalc()
{
	double view_size = (double)VIEW_SIZE;
	double view_r = view_size / 2;
	double angle_step = (90.0 / (double)Q_ANGLES);

	double angle;

	double grid_size = GRID_SIZE_EXT;
	double radius = grid_size / 2;

	point center;
	center.x = grid_size / 2;
	center.y = grid_size / 2;

	point horiz_start;
	point horiz_end;
	point vert_start;
	point vert_end;

	// https://www.triangle-calculator.com/?q=c%3D64%2C+C%3D90%2C+B%3D22.5&submit=Solve

	// For each angle
	byte q_angle = 0;
	angle = 0;
	for (q_angle = 0; q_angle<ALL_ANGLES ; ++q_angle)
	{
		// check quadrant
		switch (q_angle / Q_ANGLES)
		{
		case 0: // nw
			horiz_start.x = 0;
			horiz_start.y = grid_size-1;
			horiz_end.x = grid_size-1;
			horiz_end.y = grid_size-1;

			vert_start.x = grid_size-1;
			vert_start.y = 0;
			vert_end.x = grid_size-1;
			vert_end.y = grid_size-1;
			break;
		case 1: // ne
			horiz_start.x = 0;
			horiz_start.y = grid_size-1;
			horiz_end.x = grid_size-1;
			horiz_end.y = grid_size-1;

			vert_start.x = 0;
			vert_start.y = 0;
			vert_end.x = 0;
			vert_end.y = grid_size-1;
			break;
		case 2: // se
			horiz_start.x = 0;
			horiz_start.y = 0;
			horiz_end.x = grid_size-1;
			horiz_end.y = 0;

			vert_start.x = 0;
			vert_start.y = 0;
			vert_end.x = 0;
			vert_end.y = grid_size-1;
			break;
		case 3: // sw
			horiz_start.x = 0;
			horiz_start.y = 0;
			horiz_end.x = grid_size-1;
			horiz_end.y = 0;

			vert_start.x = grid_size-1;
			vert_start.y = 0;
			vert_end.x = grid_size-1;
			vert_end.y = grid_size-1;
			break;
		}
		point diameter_start;
		point diameter_end;
		diameter_start = point_from_angle(center, radius, angle);
		diameter_end = point_from_angle(center, radius, angle + 180);

		// For each point on diameter (2r) divided by view_size
		double diameter_len = distance(diameter_end, diameter_start);
		double dx = diameter_end.x - diameter_start.x;
		double dy = diameter_end.y - diameter_start.y;
		double step_x = dx / (double)VIEW_SIZE;
		double step_y = dy / (double)VIEW_SIZE;
			
		point ray_start;
		ray_start = diameter_start;
		int i;
		for (i=0;i<VIEW_SIZE;++i)
		{
			point ray_end = point_from_angle(ray_start, radius, angle + 90);

			point intersection_horiz;
			point intersection_vert;

			// 1. Calculate intersection with vertical 
			// 2. Calculate intersection with horizontal

			intersection_horiz = calc_intersection(ray_start, ray_end, horiz_start, horiz_end);
			intersection_vert = calc_intersection(ray_start, ray_end, vert_start, vert_end);

			// Use the closer one
			double distance_horiz = distance(ray_start, intersection_horiz);
			if (isnan(distance_horiz))
				distance_horiz = DBL_MAX;

			double distance_vert = distance(ray_start, intersection_vert);
			if (isnan(distance_vert))
				distance_vert = DBL_MAX;

			if (distance_horiz < distance_vert)
				ray_end = intersection_horiz;
			else
				ray_end = intersection_vert;

			if (sizeof(view_point_x[q_angle][i]) == 1) // byte
			{
				if (ray_end.x > 255)
					ray_end.x = 255;
				if (ray_end.y > 255)
					ray_end.y = 255;
			}
			view_point_x[q_angle][i] = ray_end.x;
			view_point_y[q_angle][i] = ray_end.y;

			// next step on the view diameter
			ray_start.x += step_x;
			ray_start.y += step_y;
		}
		// calculate ray
		point step_start;
		step_start.x = 0;
		step_start.y = 0;
		point step = point_from_angle(step_start, SIZE_MUL, angle - 90);
		ray_step_x[q_angle] = (step.x); // floor(
		ray_step_y[q_angle] = (step.y);
		angle += angle_step;
	}
	return;
}

void save_as_bmp(byte view[LAYERS][VIEW_SIZE], int anim_frame)
{
	char filename[128];
	bmp_img img;

	snprintf(filename, sizeof(filename), "frame%02d.bmp", anim_frame);
	bmp_img_init_df(&img, VIEW_SIZE, LAYERS);

	for (int l = 0; l < LAYERS; ++l)
	{
		for (int v = 0; v < VIEW_SIZE; ++v)
		{
			byte c = view[l][v];
			switch (c)
			{
			case 0:
				bmp_pixel_init(&img.img_pixels[l][v], 0, 0, 0);
				break;
			case 1:
				bmp_pixel_init(&img.img_pixels[l][v], 80, 80, 80);
				break;
			case 2:
				bmp_pixel_init(&img.img_pixels[l][v], 160, 160, 160);
				break;
			case 3:
				bmp_pixel_init(&img.img_pixels[l][v], 255, 255, 255);
				break;
			}

		}
	}


	bmp_img_write(&img, filename);
	bmp_img_free(&img);
}

void calc_view_for_angle(int view_angle, int anim_frame)
{
	const int sx = 0;
	const int sy = 0;

	double pos_x, pos_y;
	byte ray_val;

	double step_x = ray_step_x[view_angle];
	double step_y = ray_step_y[view_angle];

	byte view[LAYERS][VIEW_SIZE];

	memset(view, GRID_EMPTY, sizeof(view));

	for (int i = 0; i < VIEW_SIZE; ++i)
	{
		pos_x = view_point_x[view_angle][i];
		pos_y = view_point_y[view_angle][i];

		for (;;)
		{
			// calculate read point from object
			byte grid_x = (byte) (pos_x / SIZE_MUL);
			byte grid_y = (byte) (pos_y / SIZE_MUL);

			//al_draw_pixel(sx+ pos_x, sy+ pos_y, al_map_rgb(255, 255, 255));
			//al_flip_display();

			for (int z = 0; z < GRID_SIZE; ++z)
			{
				ray_val = object[z][grid_y][grid_x];		

				if (ray_val != GRID_EMPTY ) // in object GRID_EMPTY is transparent so we ignore it
					view[z][i] = ray_val;
			}
			
			
			pos_x += step_x;
			pos_y += step_y;
			if (pos_x<0 || pos_x>GRID_SIZE_EXT ||
				pos_y<0 || pos_y>GRID_SIZE_EXT)
				break;
		}
	}

	// save view as BMP
	save_as_bmp(view, anim_frame);

	// convert view to anim
	for (int l = 0; l < LAYERS; ++l)
	{
		for (int v = 0; v < VIEW_SIZE; v += 4)
		{
			byte c1 = view[l][v];
			byte c2 = view[l][v + 1];
			byte c3 = view[l][v + 2];
			byte c4 = view[l][v + 3];
			byte output = 0;
			output += c1 << 6;
			output += c2 << 4;
			output += c3 << 2;
			output += c4;
			anim[anim_frame][l * 16 + v / 4] = output;
		}
	}
}

void calc_all_angles()
{
	for (int i = 0; i < ALL_ANGLES; ++i)
	{
	}

}


void load_object(char *filename)
{
	char line[1024];
	FILE* fp = fopen(filename, "rt");
	// read 3 header lines
	fgets(line, 1024, fp);
	fgets(line, 1024, fp);
	fgets(line, 1024, fp);
	int x;
	int y;
	int z;
	int new_x, new_y, new_z;
	byte c;
	char color[9]; // potentially for RGBA?
	memset(object, GRID_EMPTY, sizeof(object));
	while (fscanf(fp, "%d %d %d %s", &x, &y, &z, color) == 4)
	{
		switch (color[0])
		{
		case 'f':
			c = 3;
			break;
		case '8':
			c = 2;
			break;
		default:
			c = 0;
			break;
		}
		new_z = GRID_SIZE - z - 1;
		// we rotate the object to have angle 0 facing the front of the robot
		new_y = y;
		new_x = GRID_SIZE - x - 1;
		object[new_z][new_y][new_x] = c;
	}
	fclose(fp);
}

int main(int argc, char* argv[])
{
	int i;
	precalc();

	// anim with hand up 
	load_object("frame-004.txt");
	for (i = 0; i < 8; ++i)
		calc_view_for_angle(i + 8, i + 0);
	for (i = 0; i < 8; ++i)
		calc_view_for_angle(i + 24, i + 16);

	load_object("frame-003.txt");
	for (i = 0; i < 8; ++i)
		calc_view_for_angle(i + 16, i + 8);
	for (i = 0; i < 8; ++i)
		calc_view_for_angle(i + 0, i + 24);

	// hands down
	load_object("frame-001.txt");
	int f = ALL_ANGLES;
	for (i = -4; i < 9; ++i)
		calc_view_for_angle(i + 8, f++);

	// move hand up

	load_object("frame-002.txt");
	calc_view_for_angle(8, ALL_ANGLES + HEAD_FRAMES);

	// wave hand
	load_object("frame-003.txt");
	calc_view_for_angle(8, ALL_ANGLES + HEAD_FRAMES + 1);

	//calc_all_angles();
	save_data();
	return 0;
}