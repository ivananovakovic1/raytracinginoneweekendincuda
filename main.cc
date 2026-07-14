#include <iostream>
#include "sphere.h"
#include "hitable_list.h"
#include "float.h"
#include "camera.h"
#include "material.h"

// Definišemo maksimalnu vrijednost za float tip podatka koji koristimo pri provjeri rastojanja pogodaka zraka.
#define MAXFLOAT FLT_MAX

// Funkcija 'color' proračunava boju za proslijeđeni zrak (ray) kroz naš 3D svijet.
// Funkcija radi rekurzivno: ako zrak pogodi neki objekat, materijal tog objekta raspršuje (scatter) zrak,
// a mi rekurzivno pozivamo 'color' za taj novi zrak i množimo rezultat sa prigušenjem (attenuation) tog materijala.
vec3 color(const ray& r, hitable *world, int depth) {
    hit_record rec;
    // Provjeravamo da li zrak pogađa neki objekat u sceni u opsegu udaljenosti [0.001, MAXFLOAT]
    if (world->hit(r, 0.001, MAXFLOAT, rec)) {
        ray scattered;
        vec3 attenuation;
        // Limitiramo dubinu rekurzije na 50 odbijanja kako bismo spriječili beskonačnu petlju (i prepunjavanje steka)
        if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
             // Rekurzivni poziv: boja je proizvod prigušenja materijala i boje novog, odbijenog zraka
             return attenuation*color(scattered, world, depth+1);
        }
        else {
            // Ako pređemo 50 odbijanja ili materijal potpuno apsorbuje svjetlost, vraćamo crnu boju
            return vec3(0,0,0);
        }
    }
    else {
        // Ako zrak promaši sve objekte u sceni, iscrtavamo pozadinski gradijent (nebo)
        vec3 unit_direction = unit_vector(r.direction());
        // Mapiramo y-koordinatu pravca zraka iz [-1, 1] u [0, 1] za potrebe linearne interpolacije
        float t = 0.5*(unit_direction.y() + 1.0);
        // Linearna interpolacija (lerp) između bijele i svijetlo plave boje
        return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
    }
}

// Funkcija generiše veliku, nasumičnu scenu sa stotinama malih sfera i tri velike, centralne sfere.
hitable *random_scene() {
    int n = 500;
    hitable **list = new hitable*[n+1];
    
    // Kreiramo tlo kao ogromnu sferu sa mat (lambertian) materijalom sive boje
    list[0] =  new sphere(vec3(0,-1000,0), 1000, new lambertian(vec3(0.5, 0.5, 0.5)));
    int i = 1;
    
    // Dvostruka petlja postavlja mrežu malih sfera na tlo
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            float choose_mat = drand48(); // Nasumično biramo vrstu materijala za svaku sferu
            // Definišemo centar male sfere sa blagim nasumičnim pomjeranjem kako ne bi stajale u strogoj rešetki
            vec3 center(a+0.9*drand48(),0.2,b+0.9*drand48());
            
            // Osiguravamo da se male sfere ne preklapaju sa tri glavne, velike sfere u centru
            if ((center-vec3(4,0.2,0)).length() > 0.9) {
                if (choose_mat < 0.8) {  // 80% šanse da sfera bude mat (diffuse/lambertian)
                    list[i++] = new sphere(center, 0.2, new lambertian(vec3(drand48()*drand48(), drand48()*drand48(), drand48()*drand48())));
                }
                else if (choose_mat < 0.95) { // 15% šanse da sfera bude od metala
                    list[i++] = new sphere(center, 0.2,
                            new metal(vec3(0.5*(1 + drand48()), 0.5*(1 + drand48()), 0.5*(1 + drand48())),  0.5*drand48()));
                }
                else {  // 5% šanse da sfera bude staklena (dielectric)
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    // Dodajemo tri velike, glavne sfere u centar scene sa tri različita materijala
    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5)); // Staklena sfera
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(vec3(0.4, 0.2, 0.1))); // Mat braon sfera
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0)); // Savršeno glatka metalna sfera

    // Vraćamo listu svih kreiranih objekata upakovanu u 'hitable_list' strukturu
    return new hitable_list(list,i);
}

int main() {
    int nx = 1200; // Širina slike u pikselima
    int ny = 800;  // Visina slike u pikselima
    int ns = 10;   // Broj uzoraka po pikselu za antialiasing (supersampling)
    
    // Ispisujemo zaglavlje PPM formata slike
    std::cout << "P3\n" << nx << " " << ny << "\n255\n";
    
    // Inicijalizujemo privremeni niz objekata za scenu
    hitable *list[5];
    float R = cos(M_PI/4);
    list[0] = new sphere(vec3(0,0,-1), 0.5, new lambertian(vec3(0.1, 0.2, 0.5)));
    list[1] = new sphere(vec3(0,-100.5,-1), 100, new lambertian(vec3(0.8, 0.8, 0.0)));
    list[2] = new sphere(vec3(1,0,-1), 0.5, new metal(vec3(0.8, 0.6, 0.2), 0.0));
    list[3] = new sphere(vec3(-1,0,-1), 0.5, new dielectric(1.5));
    list[4] = new sphere(vec3(-1,0,-1), -0.45, new dielectric(1.5));
    hitable *world = new hitable_list(list,5);
    
    // Ovdje prebrisujemo gornju jednostavnu scenu i generišemo veliku, kompleksnu nasumičnu scenu
    world = random_scene();

    // Podešavanje parametara kamere (pozicija, fokus, otvor blende za dubinsku oštrinu)
    vec3 lookfrom(13,2,3); // Pozicija kamere u prostoru
    vec3 lookat(0,0,0);    // Tačka u koju kamera gleda
    float dist_to_focus = 10.0; // Udaljenost na kojoj je slika u savršenom fokusu
    float aperture = 0.1;       // Otvor blende (veća vrijednost daje zamućeniju pozadinu/veći bokeh efekat)

    // Kreiramo objekat kamere sa definisanim parametrima i vertikalnim uglom vidnog polja od 20 stepeni
    camera cam(lookfrom, lookat, vec3(0,1,0), 20, float(nx)/float(ny), aperture, dist_to_focus);

    // Glavna petlja za renderovanje koja prolazi kroz sve piksele slike (od vrha ka dnu, s lijeva na desno)
    for (int j = ny-1; j >= 0; j--) {
        for (int i = 0; i < nx; i++) {
            vec3 col(0, 0, 0); // Akumulator boje za trenutni piksel
            
            // Antialiasing: za svaki piksel ispaljujemo 'ns' nasumično pomjerenih zraka
            for (int s=0; s < ns; s++) {
                // drand48() nam omogućava da uzmemo podpikselne koordinate za glatke prelaze na ivicama
                float u = float(i + drand48()) / float(nx);
                float v = float(j + drand48()) / float(ny);
                ray r = cam.get_ray(u, v);
                vec3 p = r.point_at_parameter(2.0); // Računamo referentnu tačku na zraku (koristi se za debug/orijentaciju)
                col += color(r, world, 0); // Pozivamo funkciju color sa početnom dubinom rekurzije 0
            }
            
            // Računamo prosječnu vrijednost sakupljene boje po pikselu
            col /= float(ns);
            // Primjenjujemo gama korekciju (gamma = 2.0, u kodu predstavljeno kroz kvadratni korijen)
            col = vec3( sqrt(col[0]), sqrt(col[1]), sqrt(col[2]) );
            
            // Skalirano boju u opseg [0, 255] i ispisujemo u standardni PPM format piksel po piksel
            int ir = int(255.99*col[0]);
            int ig = int(255.99*col[1]);
            int ib = int(255.99*col[2]);
            std::cout << ir << " " << ig << " " << ib << "\n";
        }
    }
}
