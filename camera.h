#ifndef CAMERAH
#define CAMERAH

#include "ray.h"

// Definisali smo klasu kamere koja nam služi da mapiramo 2D piksele sa ekrana u 3D zrake u prostoru.
class camera {
    public:
        // Konstruktor kamere 
	    camera() {
            lower_left_corner = vec3(-2.0, -1.0, -1.0); // Donji lijevi ugao našeg virtuelnog ekrana (viewport-a)
            horizontal = vec3(4.0, 0.0, 0.0);           // Širina ekrana kroz 3D prostor
            vertical = vec3(0.0, 2.0, 0.0);             // Visina ekrana kroz 3D prostor
            origin = vec3(0.0, 0.0, 0.0);               // Kamera je postavljena tačno u koordinatni početak (0,0,0)
        }

        // Ova funkcija nam je ključna jer za svake koordinate (u, v) na ekranu generiše zrak.
        // Parametri 'u' i 'v' su normalizovane vrijednosti (od 0 do 1) koje nam govore gdje se tačno 
        // na ekranu nalazi piksel koji trenutno renderujemo na GPU.
        ray get_ray(float u, float v) { 
            // Zrak kreće iz oka kamere (origin) i ide u pravcu tačke na ekranu koju smo dobile 
            // sabiranjem početnog ugla i pomaka po horizontalnoj i vertikalnoj osi.
            return ray(origin, lower_left_corner + u*horizontal + v*vertical - origin); 
        }

        vec3 origin;
        vec3 lower_left_corner;
        vec3 horizontal;
        vec3 vertical;
};

#endif
