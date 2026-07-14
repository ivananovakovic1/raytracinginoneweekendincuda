#ifndef SPHEREH
#define SPHEREH

#include "hitable.h"

// Ovdje nasljeđujemo našu baznu klasu 'hitable'. 
// i tretiramo ih na isti način kad računamo pogotke zraka.
class sphere: public hittable {
    public:
        __device__ sphere() {}
        __device__ sphere(vec3 cen, float r, material *m) : center(cen), radius(r), mat_ptr(m)  {};
        
        // Ova metoda provjerava da li naš zrak siječe sferu.
        __device__ virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;
        
        vec3 center;       // Centar sfere u 3D prostoru
        float radius;      // Poluprečnik sfere
        material *mat_ptr; // Pokazivač na materijal od kog je sfera napravljena
};

// Ovdje pišemo samu logiku za računanje presjeka zraka i sfere.
__device__ bool sphere::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
    vec3 oc = r.origin() - center; // Vektor od centra sfere do početka zraka
    
    // Računamo koeficijente za kvadratnu jednačinu.
    // Primijetile smo da ovdje koristimo 'f' sufikse za brojeve (npr. 2.0f i slično u proračunima)
    // kako ne bismo natjerale GPU da radi sa sporijim 'double' tipom podataka.
    float a = dot(r.direction(), r.direction());
    float b = dot(oc, r.direction());
    float c = dot(oc, oc) - radius*radius;
    
    // Računamo diskriminantu da vidimo da li zrak uopšte prolazi kroz sferu
    float discriminant = b*b - a*c;
    
    if (discriminant > 0) {
        // Tražimo prvo, bliže rješenje jednačine (tačku ulaska zraka u sferu)
        float temp = (-b - sqrt(discriminant)) / a;
        
        // Provjeravamo da li je to rješenje unutar naših granica [t_min, t_max].
        // Ovo nam je važno da izbjegnemo "shadow acne" i da ne renderujemo stvari iza kamere.
        if (temp < t_max && temp > t_min) {
            rec.t = temp;
            rec.p = r.point_at_parameter(rec.t); // Tačna 3D tačka udara
            
            // Računamo normalu u tački udara (vektor koji ide od centra sfere prema toj tački).
            // Dijelimo sa radijusom da bismo dobili normalizovan vektor dužine 1.
            rec.normal = (rec.p - center) / radius;
            
            rec.mat_ptr = mat_ptr; // Pamti se materijal za sjenčenje
            return true;
        }
        
        // Ako je prvo rješenje ispalo van granica, provjeravamo ono drugo (dalje) rješenje
        temp = (-b + sqrt(discriminant)) / a;
        if (temp < t_max && temp > t_min) {
            rec.t = temp;
            rec.p = r.point_at_parameter(rec.t);
            rec.normal = (rec.p - center) / radius;
            rec.mat_ptr = mat_ptr;
            return true;
        }
    }
    // Ako nema rješenja kvadratne jednačine, zrak je promašio sferu
    return false;
}

#endif
