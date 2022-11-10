//
// Created by Mario Marchand on 16-12-29.
//

#include "DonneesGTFS.h"

using namespace std;

//! \brief Pour string_to_vector ci-dessous
//! \brief Solution obtenue de https://www.techiedelight.com/
std::string ltrim(const std::string &s, const std::string &WHITESPACE)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

//! Pour string_to_vector ci-dessous
//! \brief Solution obtenue de https://www.techiedelight.com/
std::string rtrim(const std::string &s, const std::string &WHITESPACE)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

//! \brief partitionne un string en un vecteur de strings
//! \brief cela modifie le string s en lui enlevant tous les guillements
//! \param[in] s: le string à être partitionner
//! \param[in] delim: le caractère utilisé pour le partitionnement
//! \return le vecteur de string sans le caractère utilisé pour le partitionnement
//! \brief On supprime tous les caractères blancs situés au début et à la fin de chaque élément du vecteur
vector<string> DonneesGTFS::string_to_vector(string &s, char delim)
{
    s.erase(remove(s.begin(), s.end(), '"'), s.end()); // enlever les guillemets du texte
    stringstream ss(s);
    string item;
    vector<string> elems;
    while (getline(ss, item, delim))
    {
        //pour chaque string item, on enlève les caractères blancs du début et de la fin
        //solution obtenue de https://www.techiedelight.com/
        auto ret_item = rtrim(ltrim(item, " \n\r\t\f\v"), " \n\r\t\f\v");

        //maintenant que les caractères blancs sont enlevés, on insère item dans le vecteur elems
        elems.push_back(ret_item);
    }
    return elems;
}

//! \brief construit un objet GTFS
//! \param[in] p_date: la date utilisée par le GTFS
//! \param[in] p_now1: l'heure du début de l'intervalle considéré
//! \param[in] p_now2: l'heure de fin de l'intervalle considéré
//! \brief Ces deux heures définissent l'intervalle de temps du GTFS; seuls les moments de [p_now1, p_now2) sont considérés
DonneesGTFS::DonneesGTFS(const Date &p_date, const Heure &p_now1, const Heure &p_now2)
        : m_date(p_date), m_now1(p_now1), m_now2(p_now2), m_nbArrets(0), m_tousLesArretsPresents(false)
{
}

unsigned int DonneesGTFS::getNbArrets() const
{
    return m_nbArrets;
}

size_t DonneesGTFS::getNbLignes() const
{
    return m_lignes.size();
}

size_t DonneesGTFS::getNbStations() const
{
    return m_stations.size();
}

size_t DonneesGTFS::getNbTransferts() const
{
    return m_transferts.size();
}

size_t DonneesGTFS::getNbStationsDeTransfert() const
{
    return m_stationsDeTransfert.size();
}

size_t DonneesGTFS::getNbServices() const
{
    return m_services.size();
}

size_t DonneesGTFS::getNbVoyages() const
{
    return m_voyages.size();
}

void DonneesGTFS::afficherLignes() const
{
    std::cout << "======================" << std::endl;
    std::cout << "   LIGNES GTFS   " << std::endl;
    std::cout << "   COMPTE = " << m_lignes.size() << "   " << std::endl;
    std::cout << "======================" << std::endl;
    for (const auto & ligneM : m_lignes_par_numero)
    {
        cout << ligneM.second;
    }
    std::cout << std::endl;
}

void DonneesGTFS::afficherStations() const
{
    std::cout << "========================" << std::endl;
    std::cout << "   STATIONS GTFS   " << std::endl;
    std::cout << "   COMPTE = " << m_stations.size() << "   " << std::endl;
    std::cout << "========================" << std::endl;
    for (const auto & stationM : m_stations)
    {
        std::cout << stationM.second << endl;
    }
    std::cout << std::endl;
}

void DonneesGTFS::afficherStationsDeTransfert() const
{
    std::cout << "================================" << std::endl;
    std::cout << "   STATIONS DE TRANSFERT GTFS   " << std::endl;
    std::cout << "   COMPTE = " << m_stationsDeTransfert.size() << "   " << std::endl;
    std::cout << "================================" << std::endl;
    for (auto station_id : m_stationsDeTransfert)
    {
        std::cout << "Station id: " << station_id << endl;
    }
    std::cout << std::endl;

}


void DonneesGTFS::afficherTransferts() const
{
    std::cout << "========================" << std::endl;
    std::cout << "   TRANSFERTS GTFS   " << std::endl;
    std::cout << "   COMPTE = " << m_transferts.size() << "   " << std::endl;
    std::cout << "========================" << std::endl;
    for (unsigned int i = 0; i < m_transferts.size(); ++i)
    {
        std::cout << "De la station " << get<0>(m_transferts.at(i)) << " vers la station " << get<1>(m_transferts.at(i))
                  <<
                  " en " << get<2>(m_transferts.at(i)) << " secondes" << endl;

    }
    std::cout << std::endl;

}


void DonneesGTFS::afficherArretsParVoyages() const
{
    ofstream myfile;
    myfile.open ("example.txt");

    myfile << "=====================================" << std::endl;
    myfile << "   VOYAGES DE LA JOURNÉE DU " << m_date << std::endl;
    myfile << "   " << m_now1 << " - " << m_now2 << std::endl;
    myfile << "   COMPTE = " << m_voyages.size() << "   " << std::endl;
    myfile << "=====================================" << std::endl;

    for (const auto & voyageM : m_voyages)
    {
        auto ligne_id = voyageM.second.getLigne();
        auto l_itr = m_lignes.find(ligne_id);
        if (l_itr == m_lignes.end())
            throw logic_error("DonneesGTFS::afficherArretsParVoyages(): ligne_id absent de m_lignes");
        myfile << (l_itr->second).getNumero() << " ";
        myfile << voyageM.second << endl;
        for (const auto & a: voyageM.second.getArrets())
        {
            auto station_id = a->getStationId();
            auto s_itr = m_stations.find(station_id);
            if (s_itr == m_stations.end())
                throw logic_error("DonneesGTFS::afficherArretsParVoyages(): station_id absent de m_stations");
            myfile << a->getHeureArrivee() << " station " << s_itr->second << endl;
        }
    }

    myfile << std::endl;
    myfile.close();
}

void DonneesGTFS::afficherArretsParStations() const
{
    std::cout << "========================" << std::endl;
    std::cout << "   ARRETS PAR STATIONS   " << std::endl;
    std::cout << "   Nombre d'arrêts = " << m_nbArrets << std::endl;
    std::cout << "========================" << std::endl;
    for ( const auto & stationM : m_stations)
    {
        std::cout << "Station " << stationM.second << endl;
        for ( const auto & arretM : stationM.second.getArrets())
        {
            auto voyage_id = arretM.second->getVoyageId();
            auto v_itr = m_voyages.find(voyage_id);
            if (v_itr == m_voyages.end())
                throw logic_error("DonneesGTFS::afficherArretsParStations(): voyage_id absent de m_voyages");
            auto ligne_id = (v_itr->second).getLigne();
            auto l_itr = m_lignes.find(ligne_id);
            if (l_itr == m_lignes.end())
                throw logic_error("DonneesGTFS::afficherArretsParStations(): ligne_id absent de m_lignes");
            std::cout << arretM.first << " - " << (l_itr->second).getNumero() << " " << v_itr->second << std::endl;
        }
    }
    std::cout << std::endl;
}

const std::map<std::string, Voyage> &DonneesGTFS::getVoyages() const
{
    return m_voyages;
}

const std::map<std::string, Station> &DonneesGTFS::getStations() const
{
    return m_stations;
}

const std::set<std::string> &DonneesGTFS::getStationsDeTransfert() const
{
    return m_stationsDeTransfert;
}

const std::vector<std::tuple<std::string, std::string, unsigned int> > &DonneesGTFS::getTransferts() const
{
    return m_transferts;
}

Heure DonneesGTFS::getTempsFin() const
{
    return m_now2;
}

Heure DonneesGTFS::getTempsDebut() const
{
    return m_now1;
}

const std::unordered_map<std::string, Ligne> &DonneesGTFS::getLignes() const
{
    return m_lignes;
}



