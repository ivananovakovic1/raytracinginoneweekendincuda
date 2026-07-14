#ifndef MATERIALH
#define MATERIALH

struct hit_record;

#include "ray.h"
#include "hitable.h"

// Schlick-ova aproksimacija (Schlick's approximation):
// Staklo (dielektrik) mijenja svoju refleksivnost u zavisnosti od ugla pod kojim ga gledamo.
// Kada gledamo ravno u staklo, refleksija je slaba, ali pod oštrim uglom staklo se ponaša skoro kao ogledalo.
__device__ float schlick(float cosine, float ref_idx) {
    float r0 = (1.0f-ref_idx) / (1.0f+ref_idx);
    r0 = r0*r0;
    return r0 + (1.0f-r0)*pow((1.0f - cosine),5.0f);
}

// Snell-ov zakon prelamanja (Snell's Law):
// Funkcija računa smjer prelomljenog zraka kroz medijum.
// Ako je diskriminanta u jednačini negativna, to znači da dolazi do "potpune unutrašnje refleksije",
// u tom slučaju prelamanje je nemoguće i svjetlost se mora u potpunosti odbiti (reflektovati).
__device__ bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) {
    vec3 uv = unit_vector(v);
    float dt = dot(uv, n);
    float discriminant = 1.0f - ni_over_nt*ni_over_nt*(1-dt*dt);
    if (discriminant > 0) {
        refracted = ni_over_nt*(uv - n*dt) - n*sqrt(discriminant);
        return true;
    }
    else
        return false;
}

// Makro koji CUDA priručnik uvodi radi lakšeg pozivanja trodimenzionalnog nasumičnog vektora na GPU.
// Svaka koordinata (x, y, z) se nezavisno generiše unutar opsega [0.0, 1.0] pomoću 'curand_uniform'.
#define RANDVEC3 vec3(curand_uniform(local_rand_state),curand_uniform(local_rand_state),curand_uniform(local_rand_state))

// Metoda odbacivanja (Rejection Method):
// Da bismo dobili ravnomjerno raspršene zrake za difuzne materijale, generišemo nasumične tačke 
// unutar jedinične sfere. Generišemo tačke unutar kocke [-1,1]^3 i odbacujemo one koje su van sfere (dužina >= 1.0).
__device__ vec3 random_in_unit_sphere(curandState *local_rand_state) {
    vec3 p;
    do {
        p = 2.0f*RANDVEC3 - vec3(1,1,1);
    } while (p.squared_length() >= 1.0f);
    return p;
}

// Formula za idealnu refleksiju (ogledalo):
// Dolazeći zrak 'v' se odbija od površine sa normalom 'n'. 
// Vektor 'n' mora biti normalizovan, a rezultat je matematički čist odraz.
__device__ vec3 reflect(const vec3& v, const vec3& n) {
     return v - 2.0f*dot(v,n)*n;
}

// Apstraktna bazna klasa za materijale.
// Svaki materijal mora implementirati funkciju 'scatter' na GPU (__device__).
class material  {
    public:
        __device__ virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered, curandState *local_rand_state) const = 0;
};

// Lambertian (idealno difuzni/mat materijal):
// Koristimo jednostavan model difuzije gdje se novi zrak odbija ka nasumičnoj tački 
// unutar sfere koja je tangentna na tačku pogotka i pomjerena duž normale (p + normal + random_vector).
class lambertian : public material {
    public:
        __device__ lambertian(const vec3& a) : albedo(a) {}
        __device__ virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered, curandState *local_rand_state) const  {
             vec3 target = rec.p + rec.normal + random_in_unit_sphere(local_rand_state);
             scattered = ray(rec.p, target-rec.p);
             attenuation = albedo; // Boja zraka biva prigušena za albedo vrijednost ovog materijala
             return true;
        }

        vec3 albedo; // Albedo predstavlja refleksivnost površine (udio odbijene svjetlosti za R, G i B kanale)
};

// Metal (reflektujući materijal sa opcionom hrapavošću):
// Savršeni metal samo reflektuje zrak (reflect). Hrapavost (fuzz) simuliramo tako što 
// krajnju tačku reflektovanog zraka blago pomjerimo dodavanjem nasumičnog vektora iz jedinične sfere skaliranog sa 'fuzz'.
class metal : public material {
    public:
        __device__ metal(const vec3& a, float f) : albedo(a) { if (f < 1) fuzz = f; else fuzz = 1; }
        __device__ virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered, curandState *local_rand_state) const  {
            vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
            scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere(local_rand_state));
            attenuation = albedo;
            // Ako hrapavost skrene zrak "ispod" površine objekta (skalarni proizvod sa normalom je <= 0), 
            // taj zrak se apsorbuje (poništava), što je fizički ispravno ponašanje.
            return (dot(scattered.direction(), rec.normal) > 0.0f);
        }
        vec3 albedo;
        float fuzz; // Parametar hrapavosti: 0.0 za savršeno ogledalo, 1.0 za maksimalno mutnu refleksiju
};

// Dielectric (staklo, voda i drugi prozirni materijali sa indeksom prelamanja 'ref_idx'):
// Ovaj materijal kombinuje refleksiju i refrakciju.
class dielectric : public material {
public:
    __device__ dielectric(float ri) : ref_idx(ri) {}
    __device__ virtual bool scatter(const ray& r_in,
                                 const hit_record& rec,
                                 vec3& attenuation,
                                 ray& scattered,
                                 curandState *local_rand_state) const  {
        vec3 outward_normal;
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        float ni_over_nt;
        attenuation = vec3(1.0, 1.0, 1.0); // Staklo ne apsorbuje svjetlost (attenuation je 1, prolazi sva energija)
        vec3 refracted;
        float reflect_prob;
        float cosine;
        
        // Provjeravamo da li zrak ulazi u objekat ili izlazi iz njega:
        if (dot(r_in.direction(), rec.normal) > 0.0f) {
            // Zrak izlazi iz gušćeg medijuma (npr. stakla) u rjeđi (vazduh)
            outward_normal = -rec.normal;
            ni_over_nt = ref_idx; // Odnos indeksa prelamanja n1/n2
            cosine = dot(r_in.direction(), rec.normal) / r_in.direction().length();
            // Prilagođavamo kosinus upadnog ugla za Schlick aproksimaciju prilikom izlaska iz medijuma
            cosine = sqrt(1.0f - ref_idx*ref_idx*(1-cosine*cosine));
        }
        else {
            // Zrak ulazi iz rjeđeg medijuma (vazduha) u gušći (staklo)
            outward_normal = rec.normal;
            ni_over_nt = 1.0f / ref_idx;
            cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
        }
        
        // Na osnovu zakona prelamanja, odlučujemo da li se zrak prelama (refract) ili dolazi do refleksije:
        if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted))
            reflect_prob = schlick(cosine, ref_idx); // Računamo vjerovatnoću refleksije
        else
            reflect_prob = 1.0f; // Došlo je do potpune unutrašnje refleksije (100% šanse za odbijanje)
            
        // Pomoću curand_uniform na GPU-u odlučujemo da li se zrak odbija ili prelama (ruski rulet metoda):
        if (curand_uniform(local_rand_state) < reflect_prob)
            scattered = ray(rec.p, reflected);
        else
            scattered = ray(rec.p, refracted);
        return true;
    }

    float ref_idx; // Indeks prelamanja (npr. vazduh = 1.0, staklo = 1.5, voda = 1.33)
};
#endif
