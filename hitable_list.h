#ifndef HITABLELISTH
#define HITABLELISTH

#include "hitable.h"

// Ova klasa nam služi da grupišemo sve objekte u sceni na jedno mjesto.
// Pošto i sama nasljeđuje 'hitable', možemo je tretirati kao jedan veliki objekat za provjeru sudara.
class hitable_list: public hitable {
    public:
        hitable_list() {}
        
        // Konstruktor prima niz pokazivača na hitable objekte (l) i veličinu tog niza (n)
        hitable_list(hitable **l, int n) { list = l; list_size = n; }
        
        // Glavna metoda koja prolazi kroz sve objekte u listi i provjerava da li ih je zrak pogodio
        virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;
        
        hitable **list; // Dinamički niz pokazivača na objekte u sceni
        int list_size;  // Broj objekata u našoj sceni
};

// Logika za prolazak kroz cijelu scenu.
// Kada tražimo šta je zrak pogodio, 
// moramo naći najbliži objekat ispred kamere.
bool hitable_list::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
    hit_record temp_rec;
    bool hit_anything = false;
    
    // Na početku, naša najbliža tačka udara ('closest_so_far') je zapravo gornja granica (t_max).
    // Kako budemo pronalazile pogotke koji su bliži, smanjivaćemo ovu granicu.
    float closest_so_far = t_max;
    
    // Prolazimo petljom kroz apsolutno sve objekte koje smo ubacile u scenu
    for (int i = 0; i < list_size; i++) {
        // Provjeravamo sudar za trenutni objekat 'list[i]'.
        // Ključni trik: umjesto fiksne granice, predajemo 'closest_so_far'.
        // Na taj način, ako smo već pogodile nešto na udaljenosti npr. t=3, 
        // potpuno ignorišemo sve druge objekte koji su dalje od toga (npr. t=5) jer su zaklonjeni.
        if (list[i]->hit(r, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t; // Ažuriramo granicu na novu, bližu tačku udara
            rec = temp_rec;              // Kopiramo sve detalje o ovom (trenutno najbližem) pogotku
        }
    }
    
    // Vraćamo 'true' samo ako smo pogodile barem jedan objekat koji je vidljiv na ekranu
    return hit_anything;
}

#endif
