#ifndef HITABLELISTH
#define HITABLELISTH

#include "hitable.h"

// hitable_list je klasa koja predstavlja kolekciju (grupu) 3D objekata.
// Nasljeđuje 'hitable' interfejs, što joj omogućava da se ponaša kao jedan jedinstveni objekat u sceni.
class hitable_list: public hitable  {
    public:
        // CUDA priručnik zahtijeva specifikator '__device__' kako bi se konstruktor 
        // mogao pozvati unutar GPU kernela prilikom kreiranja objekata na grafičkoj kartici.
        __device__ hitable_list() {}
        
        // Konstruktor koji prima niz pokazivača na hitable objekte i veličinu tog niza (n).
        // Na GPU-u se ovi pokazivači obično inicijalizuju unutar setup kernela.
        __device__ hitable_list(hitable **l, int n) {list = l; list_size = n; }
        
        // Virtuelna funkcija za detekciju presjeka zraka sa bilo kojim objektom iz liste.
        // Raspon [tmin, tmax] definiše interval parametra 't' unutar kojeg se presjek smatra validnim.
        __device__ virtual bool hit(const ray& r, float tmin, float tmax, hit_record& rec) const;
        
        hitable **list; // Niz pokazivača na objekte koji implementiraju 'hitable' interfejs
        int list_size;  // Broj objekata u listi
};

// Implementacija funkcije za detekciju presjeka (hit) na GPU.
// Kada zrak prolazi kroz scenu, moramo provjeriti presjek sa svim objektima,
// ali prikazati samo onaj koji je najbliži početku zraka (closest intersection).
__device__ bool hitable_list::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
        hit_record temp_rec;
        bool hit_anything = false;
        
        // 'closest_so_far' na početku postavljamo na gornju granicu intervala (t_max).
        // Kako pronalazimo pogotke koji su bliži, ova granica se smanjuje. Na taj način osiguravamo
        // da se provjeravaju samo oni presjeci koji su bliži od do sada najbiližeg pronađenog.
        float closest_so_far = t_max;
        
        // Iteriramo kroz sve objekte u našoj sceni na GPU
        for (int i = 0; i < list_size; i++) {
            // Pozivamo 'hit' funkciju za pojedinačni objekat, prosljeđujući 'closest_so_far' kao novu gornju granicu.
            if (list[i]->hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t; // Smanjujemo interval jer smo pronašli bliži presjek
                rec = temp_rec;              // Ažuriramo hit_record sa detaljima o najbližem pogotku (tačka, normala, materijal)
            }
        }
        return hit_anything;
}

#endif
