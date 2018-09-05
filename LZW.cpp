#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <limits>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>


//Maksimalna velicina rjecnika (kada se rjecnik napuni, resetujemo ga)
const uint16_t granica { std::numeric_limits<uint16_t>::max() };
const long int ascii_prvi = std::numeric_limits<char>::min();
const long int ascii_zadnji = std::numeric_limits<char>::max();

//reseovanje liste za kompresiju(mape)
void reset_map(std::map<std::vector<char>, uint16_t> &rjecnik)
{
    rjecnik.clear();

    for(long int simbol = ascii_prvi; simbol <= ascii_zadnji; ++simbol)
    {
        const uint16_t velicina_rjecnika = rjecnik.size();
        std::vector<char> v;
        v.push_back(static_cast<char> (simbol));
        rjecnik[v] = velicina_rjecnika;
    }
}

//reseovanje liste za dekompresiju(vektora)
void reset_vector(std::vector<std::vector<char>> &rjecnik)
{
    rjecnik.clear();
    rjecnik.reserve(granica);

    for(long int simbol = ascii_prvi; simbol <= ascii_zadnji; ++simbol) rjecnik.push_back({static_cast<char> (simbol)});
}

//Preklopljeni operator za spajanje stringova
//Vektor koristimo jer string nije pouzdan za pohranu i obradu podataka na bajt nivou.
std::vector<char> operator + (std::vector<char> recenica, char slovo)
{
    recenica.push_back(slovo);
    return recenica;
}

//Podatke sa ulaznog toka kompresujemo i zapisujemo na izlazni tok
void Enkoder(std::istream &ulazni_tok, std::ostream &izlazni_tok)
{
    std::map<std::vector<char>, uint16_t> rjecnik;

    reset_map(rjecnik);

    std::vector<char> sekvenca;
    char simbol;

    while (ulazni_tok.get(simbol))
    {
        if (rjecnik.size() ==  granica) reset_map(rjecnik);

        sekvenca.push_back(simbol);

        if (rjecnik.count(sekvenca) == 0)
        {
            const uint16_t velicina_rjecnika = rjecnik.size();

            rjecnik[sekvenca] = velicina_rjecnika;
            sekvenca.pop_back();
            izlazni_tok.write(reinterpret_cast<const char *> (&rjecnik.at(sekvenca)), sizeof (uint16_t));
            sekvenca = { simbol };
        }
    }

    if (!sekvenca.empty())
        izlazni_tok.write(reinterpret_cast<const char *> (&rjecnik.at(sekvenca)), sizeof (uint16_t));
}

//podatke sa ulaznog toka dekompresujemo i zapisujemo na izlazni tok
void Dekoder(std::istream &ulazni_tok, std::ostream &izlazni_tok)
{
    std::vector<std::vector<char>> rjecnik;

    reset_vector(rjecnik);

    std::vector<char> sekvenca;
    uint16_t kod;

    while (ulazni_tok.read(reinterpret_cast<char *> (&kod), sizeof (uint16_t)))
    {
        if (rjecnik.size() ==  granica) reset_vector(rjecnik);

        if (kod > rjecnik.size()) throw std::runtime_error("Pogresno zakodirana sekvenca!");

        if(kod == rjecnik.size()) rjecnik.push_back(sekvenca + sekvenca.front());
        else if(!sekvenca.empty()) rjecnik.push_back(sekvenca + rjecnik.at(kod).front());

        izlazni_tok.write(&rjecnik.at(kod).front(), rjecnik.at(kod).size());
        sekvenca = rjecnik.at(kod);
    }

    if (!ulazni_tok.eof() || ulazni_tok.gcount() != 0) throw std::runtime_error("Kompresovani fajl nije ispravan!");
}


int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        std::cout << "Pogresan broj argumenata!";
        return 0;
    }

    char komanda(' ');

    if (std::string(argv[1]) == "-c") komanda = 'c';
    else if (std::string(argv[1]) == "-d") komanda = 'd';
    else
    {
        std::cout << "Naredba " + std::string(argv[1]) + " nije poznata!(dostupne naredbe -c i -d)!";
        return 0;
    }

    std::ifstream ulazni_fajl(argv[2], std::ios_base::binary);

    if (!ulazni_fajl.is_open())
    {
        std::cout << "Fajl " + std::string(argv[2]) + " se ne moze otvoriti!";
        return 0;
    }

    std::ofstream izlazni_fajl(argv[3], std::ios_base::binary);

    if (!izlazni_fajl.is_open())
    {
        std::cout << "Fajl " + std::string(argv[3]) + " se ne moze otvoriti!";
        return 0;
    }


    try
    {
        ulazni_fajl.exceptions(std::ios_base::badbit);
        izlazni_fajl.exceptions(std::ios_base::badbit | std::ios_base::failbit);

        if (komanda == 'c') Enkoder(ulazni_fajl, izlazni_fajl);
        else if (komanda == 'd') Dekoder(ulazni_fajl, izlazni_fajl);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Greska: " << e.what();
        return 0;
    }

    return 0;
}
