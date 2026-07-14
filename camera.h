#ifndef CAMERAH
#define CAMERAH

#include <curand_kernel.h>
#include "ray.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Pomoćna funkcija koja generiše nasumičnu tačku unutar jediničnog diska (kruga).
// Koristimo je za simulaciju kružnog otvora blende (aperture) kako bismo dobili prirodan, kružni bokeh efekat (zamućenje van fokusa).
// Pošto se izvršava na GPU, koristi curand_uniform za generisanje nasumičnih brojeva na grafičkoj kartici.
__device__ vec3 random_in_unit_disk(curandState *local_rand_state) {
    vec3 p;
    do {
        // Generišemo tačku u kvadratu [-1, 1]x[-1, 1] i provjeravamo da li upada unutar kruga (disk)
        p = 2.0f*vec3(curand_uniform(local_rand_state),curand_uniform(local_rand_state),0) - vec3(1,1,0);
    } while (dot(p,p) >= 1.0f); // Ako je van kruga, ponavljamo postupak (metoda odbacivanja)
    return p;
}

class camera {
public:
    // Konstruktor kamere koji se izvršava na GPU (__device__).
    // Računa geometriju vidnog polja na osnovu pozicije posmatrača, tačke gledanja, vidnog ugla (vfov) i odnosa stranica slike (aspect).
    __device__ camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect, float aperture, float focus_dist) { // vfov je vertikalni ugao u stepenima
        lens_radius = aperture / 2.0f; // Poluprečnik sočiva (veći poluprečnik stvara pliće polje fokusa i jači bokeh)
        float theta = vfov*((float)M_PI)/180.0f; // Pretvaramo stepene u radijane
        float half_height = tan(theta/2.0f);     // Polovina visine projekcione ravni na jediničnoj udaljenosti
        float half_width = aspect * half_height; // Polovina širine projekcione ravni
        
        origin = lookfrom; // Pozicija oka (kamere) u 3D prostoru
        
        // Kreiramo ortonormirani bazni sistem (u, v, w) za kameru:
        w = unit_vector(lookfrom - lookat); // Z-osa kamere (gleda suprotno od smjera snimanja)
        u = unit_vector(cross(vup, w));     // X-osa kamere (desno u odnosu na ekran)
        v = cross(w, u);                    // Y-osa kamere (gore u odnosu na ekran)
        
        // Računamo donji lijevi ugao ekrana u 3D prostoru, uzimajući u obzir distancu fokusiranja (focus_dist)
        lower_left_corner = origin  - half_width*focus_dist*u -half_height*focus_dist*v - focus_dist*w;
        
        // Definišemo vektore koji se protežu duž širine i visine ekrana
        horizontal = 2.0f*half_width*focus_dist*u;
        vertical = 2.0f*half_height*focus_dist*v;
    }
    
    // Funkcija get_ray generiše zrak za proslijeđene koordinatne parametre (s, t) na ekranu.
    // Koristi lokalno stanje generatora slučajnih brojeva (curandState) za proračun skretanja zraka kroz virtuelno sočivo.
    __device__ ray get_ray(float s, float t, curandState *local_rand_state) {
        // Generišemo nasumični ofset na površini sočiva kako bismo simulirali dubinsku oštrinu (depth of field)
        vec3 rd = lens_radius*random_in_unit_disk(local_rand_state);
        vec3 offset = u * rd.x() + v * rd.y(); // Pomjeramo početnu tačku zraka u odnosu na osu kamere
        
        // Zrak kreće sa pomjerene tačke na sočivu (origin + offset) i ide kroz odgovarajući piksel na ravni fokusa
        return ray(origin + offset, lower_left_corner + s*horizontal + t*vertical - origin - offset);
    }

    // Varijable članice klase koje čuvaju položaj i orijentaciju kamere
    vec3 origin;
    vec3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    float lens_radius;
};

#endif
