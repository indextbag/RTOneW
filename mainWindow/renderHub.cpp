#include "RTOneW/core.h"
#include "mainWindow.h"
#include <thread>


mainWindow *win;
bool rendering = false;
int spp;


void setInstanceForRenderDistribution(mainWindow* w) {
	win = w;
}


void imageOutput(vec3 **pix, int s, int width, int height) {
	
	win->RTOneW_process_signal(pix,s,width, height);
}

void process_image(vec3 **pix, int s, int width, int height) {
	
	QImage image(width, height, QImage::Format_RGB32);
	
	#pragma omp parallell for
	for (int row = 0; row <height; row++) {
		for (int col = 0; col < width; col++) {

			int ir = int(255.99*pix[col][row][0] / s);
			int ig = int(255.99*pix[col][row][1] / s);
			int ib = int(255.99*pix[col][row][2] / s);

			image.setPixel(col, (height - 1) - row, qRgb(ir, ig, ib));
		}
	}

	win->drawImage(image, s, spp);
}

vec3 color(const ray& r, hitable *world, int depth) {

	hit_record rec;

	if (world->hit(r, 0.001, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;
		if (depth<50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
			return attenuation * color(scattered, world, depth + 1);

		}
		else return vec3(0, 0, 0);
	}
	else {
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5*(unit_direction.y() + 1.0);

		return (1.0 - t)*vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
	}

}

void render(int width, int height, int spp, float fov, float aperture) {
	
	vec3 **pix = new vec3*[width];
	for (int i = 0; i < width; i++) {
		pix[i] = new vec3[height];
	}

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {

			pix[col][row] = vec3(0, 0, 0);
		}
	}


	hitable *list[4];
	list[0] = new sphere(vec3(0, 0, 0), 0.5, new lambertian(vec3(0.9, 0, 0)));
	list[1] = new sphere(vec3(0, -100.5, -1), 100, new lambertian(vec3(0.0, 0.9, 0.9)));
	list[2] = new sphere(vec3(0, 0, -1), 0.5, new metal(vec3(0.9, 0.9, 0.9), .1));
	list[3] = new sphere(vec3(0, 0, 1), 0.5, new dielectric(1.333));

	hitable *world = new hitable_list(list, 4);

	vec3 lookfrom(-1, 0, 4);
	vec3 lookat(0, 0, 0);
	float dist = (lookat - lookfrom).length();

	camera cam(lookfrom, lookat, vec3(0, 1, 0), fov, float(width) / float(height), aperture, dist, 0.0,1.0);
	
	for (int s = 1; s <= spp; s++) {

	#pragma omp parallel for schedule(dynamic)
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				vec3 col(0, 0, 0);
				float u = (float(i) + RND) / float(width);
				float v = (float(j) + RND) / float(height);
				ray r = cam.get_ray(u, v);
				vec3 p = r.point_at_parameter(2.0);
				col = color(r, world, 0);
				pix[i][j] = pix[i][j] + col;
			}
		}

		imageOutput(pix, s, width, height);

	}

}

void send_to_render(int width, int height, int spp, float fov, float aperture) {
	//qDebug() << "send to render";
	spp = spp;
	std::thread t(render, width, height, spp, fov, aperture);
	t.detach();
}
