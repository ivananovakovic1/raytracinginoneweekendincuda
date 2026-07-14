#ifndef MATERIALH
#define MATERIALH

struct hit_record;

#include "ray.h"
#include "hittable.h"

// Pomoćna funkcija za računanje odbijanja (refleksije) svjetlosti.
// Kada se zrak odbije od savršenog ogledala, upadni ugao je jednak uglu odbijanja.
// Ova formula radi upravo tu vektorsku matematiku.
vec3 reflect(const vec3& v, const vec3& n) {
     return v - 2*dot(v,n)*n;
}

// Pomoćna funkcija za računanje prelamanja (refrakcije) svjetlosti kroz staklene površine.
// Koristimo Shnellov zakon da izračunamo novi pravac zraka kada prelazi iz jednog medijuma u drugi (npr. iz vazduha u staklo).
bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) {
    vec3 uv = unit_vector(v);
    float dt = dot(uv, n);
    float discriminant = 1.0 - ni_over_nt*ni_over_nt*(1-dt*dt);
    if (discriminant > 0) {
        refracted = ni_over_nt*(uv - n*dt) - n*sqrt(discriminant);
        return true;
    }
    else
        return false;
}

// Schlickova aproksimacija – staklo se ponaša kao ogledalo kada ga gledamo pod veoma oštrim uglom.
// Ova formula nam daje vjerodostojan koeficijent refleksije u zavisnosti od ugla posmatranja, a brza je za računanje.
float schlick(float cosine, float ref_idx) {
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine), 5);
}

// Bazna klasa za sve materijale. Svaki materijal mora da implementira funkciju 'scatter'.
class material  {
    public:
        // Funkcija 'scatter' nam govori kako se upadni zrak (r_in) odbija ili prelama kada pogodi površinu (rec).
        // Ako se svjetlost apsorbuje, funkcija vraća false. Ako se odbije, vraća true, popunjava 'scattered' 
        // (novi zrak koji nastavlja put) i 'attenuation' (koliko je svjetlosti i u kojim bojama preživjelo udarac).
        virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const = 0;
};

// Mat materijali.
// Svjetlost se odbija u potpuno nasumičnim pravcima.
class lambertian : public material {
    public:
        lambertian(const vec3& a) : albedo(a) {}
        virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {
            // Generišemo nasumičnu tačku na jediničnoj sferi da bismo dobili prirodno, difuzno odbijanje.
            vec3 target = rec.p + rec.normal + random_in_unit_sphere();
            scattered = ray(rec.p, target-rec.p);
            attenuation = albedo; // Boja materijala definiše koliko se koje komponente svjetlosti odbilo
            return true;
        }

        vec3 albedo; // Osnovna boja materijala
};

// Metalni materijali.
// Svjetlost se odbija kao od ogledala, ali možemo dodati i faktor "zamućenosti" (fuzz).
class metal : public material {
    public:
        metal(const vec3& a, float f) : albedo(a) { if (f < 1) fuzz = f; else fuzz = 1; }
        virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {
            // Prvo računamo savršenu refleksiju zraka
            vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
            // Zatim dodajemo malo šuma (fuzz * nasumična tačka) da bismo dobili matiran metal (brušeni aluminijum npr.)
            scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere());
            attenuation = albedo;
            // Odbijanje ima smisla samo ako novi zrak ide prema vani u odnosu na površinu (ugao sa normalom > 0)
            return (dot(scattered.direction(), rec.normal) > 0);
        }

        vec3 albedo;
        float fuzz; // Parametar hrapavosti (0 = savršeno ogledalo, 1 = skroz mutno)
};

// Stakleni/providni materijali (Dielektrici).
// Ovdje se svjetlost i odbija i prelama u isto vrijeme.
class dielectric : public material {
    public:
        dielectric(float ri) : ref_idx(ri) {}
        virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {
            vec3 outward_normal;
            vec3 reflected = reflect(r_in.direction(), rec.normal);
            float ni_over_nt;
            attenuation = vec3(1.0, 1.0, 1.0); // Staklo ne gubi boju, propušta svu svjetlost
            vec3 refracted;
            float reflect_prob;
            float cosine;
            
            // Provjeravamo da li zrak ulazi u objekat ili izlazi iz njega kako bismo pravilno postavili normale i indekse prelamanja
            if (dot(r_in.direction(), rec.normal) > 0) {
                 outward_normal = -rec.normal;
                 ni_over_nt = ref_idx;
                 cosine = ref_idx * dot(r_in.direction(), rec.normal) / r_in.direction().length();
            }
            else {
                 outward_normal = rec.normal;
                 ni_over_nt = 1.0 / ref_idx;
                 cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
            }
            
            // Ako je prelamanje fizički moguće, računamo vjerovatnoću refleksije preko Schlickove formule
            if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted)) {
                reflect_prob = schlick(cosine, ref_idx);
            }
            else {
                reflect_prob = 1.0; // Dolazi do potpune unutrašnje refleksije (total internal reflection)
            }
            
            // Na osnovu vjerovatnoće (koristeći nasumičan broj) odlučujemo da li se zrak odbija ili prelama.
            // Umjesto da granamo zrak na dva dijela (što bi zagušilo memoriju),
            // mi probabilistički biramo samo jedan put.
            if (drand48() < reflect_prob) {
                scattered = ray(rec.p, reflected);
            }
            else {
                scattered = ray(rec.p, refracted);
            }
            return true;
        }

        float ref_idx; // Indeks prelamanja (npr. vazduh = 1.0, staklo = 1.5)
};

#endif
