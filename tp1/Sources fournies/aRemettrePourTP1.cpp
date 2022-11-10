//
// Created by Mario Marchand
//

#include "DonneesGTFS.h"

using namespace std;


//! \brief ajoute les lignes dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les lignes
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterLignes(const std::string &p_nomFichier)
{
    std:ifstream lignes_fichier(p_nomFichier);

    string ligne;
    getline(lignes_fichier, ligne);
    try {
        while (getline(lignes_fichier, ligne)) {
            vector<string> informationLigne = string_to_vector(ligne, ',');
            m_lignes.insert({
                    informationLigne[0],
                    Ligne(
                            informationLigne[0],
                            informationLigne[2],
                            informationLigne[4],
                            Ligne::couleurToCategorie(informationLigne[7]))
                            });

            m_lignes_par_numero.insert({
                informationLigne[2],
                Ligne(
                        informationLigne[0],
                        informationLigne[2],
                        informationLigne[4],
                        Ligne::couleurToCategorie(informationLigne[7]))
                    });
        }
    }
    catch (...){
        throw logic_error("Le fichier " + p_nomFichier + "  n'a pas été lu correctement");
    }
}

//! \brief ajoute les stations dans l'objet GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les stations
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterStations(const std::string &p_nomFichier)
{
    ifstream stations_fichier(p_nomFichier);
    string station;
    getline(stations_fichier, station);

    try {
        while (getline(stations_fichier, station)) {
            vector<string> informationStation = string_to_vector(station, ',');
            m_stations.insert({
                informationStation[0],
                Station(
                        informationStation[0],
                        informationStation[2],
                        informationStation[3],
                        Coordonnees(stod(informationStation[4]), stod(informationStation[5]) )
                        )
            });
        }
    }
    catch (...){
        throw logic_error("Le fichier " + p_nomFichier + " n'a pas été lu correctement");
    }
}

//! \brief ajoute les transferts dans l'objet GTFS
//! \breif Cette méthode doit âtre utilisée uniquement après que tous les arrêts ont été ajoutés
//! \brief les transferts (entre stations) ajoutés sont uniquement ceux pour lesquelles les stations sont prensentes dans l'objet GTFS
//! \brief les transferts sont ajoutés dans m_transferts
//! \brief les from_station_id des stations de m_transferts sont ajoutés dans m_stationsDeTransfert
//! \param[in] p_nomFichier: le nom du fichier contenant les station
//! \throws logic_error si un problème survient avec la lecture du fichier
//! \throws logic_error si tous les arrets de la date et de l'intervalle n'ont pas été ajoutés
void DonneesGTFS::ajouterTransferts(const std::string &p_nomFichier)
{
    if(m_tousLesArretsPresents)
    {
        ifstream transferts_fichier(p_nomFichier);
        string transfert;
        getline(transferts_fichier, transfert);

        try {
            while (getline(transferts_fichier, transfert)) {
                vector<string> informationTransfert = string_to_vector(transfert, ',');
                if(m_stations.find(informationTransfert[0]) != m_stations.end() && m_stations.find(informationTransfert[1]) != m_stations.end())
                {
                    unsigned int delai_attente = stoi(informationTransfert[3]) != 0 ? stoi(informationTransfert[3]) : 1;
                    m_transferts.push_back((make_tuple(informationTransfert[0], informationTransfert[1], delai_attente)));

                    if(m_stationsDeTransfert.find(informationTransfert[0]) == m_stationsDeTransfert.end() )
                    {
                        m_stationsDeTransfert.insert(informationTransfert[0]);
                    }
                }
            }
        }
        catch (...){
            throw logic_error("Le fichier " + p_nomFichier + " n'a pas été lu correctement");
        }
    }
}


//! \brief ajoute les services de la date du GTFS (m_date)
//! \param[in] p_nomFichier: le nom du fichier contenant les services
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterServices(const std::string &p_nomFichier)
{
    ifstream services_fichier(p_nomFichier);
    string service;
    getline(services_fichier, service);

    try {
        while (getline(services_fichier, service)) {
            vector<string> informationService = string_to_vector(service, ',');
            Date temp(stoi(informationService[1].substr(0,4)),
                      stoi(informationService[1].substr(4,2)),
                      stoi(informationService[1].substr(6,2)));

            if(m_date == temp && informationService[2]=="1"){
                m_services.insert(informationService[0]);
            }
        }
    }
    catch (...){
        throw logic_error("Le fichier " + p_nomFichier + " n'a pas été lu correctement");
    }
}

//! \brief ajoute les voyages de la date
//! \brief seuls les voyages dont le service est présent dans l'objet GTFS sont ajoutés
//! \param[in] p_nomFichier: le nom du fichier contenant les voyages
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterVoyagesDeLaDate(const std::string &p_nomFichier)
{
    ifstream voyages_fichier(p_nomFichier);
    string voyage;
    getline(voyages_fichier, voyage); //pour passer la première ligne

    try {
        while (getline(voyages_fichier, voyage)) {
            vector<string> informationVoyage = string_to_vector(voyage, ',');

            if(m_services.find(informationVoyage[1]) != m_services.end()){
                m_voyages.insert({
                    informationVoyage[3],
                    Voyage(
                            informationVoyage[3],
                            informationVoyage[0],
                            informationVoyage[1],
                            informationVoyage[4]
                            )});
            }
        }
    }
    catch (...){
        throw logic_error("Le fichier " + p_nomFichier + " n'a pas été lu correctement");
    }
}

//! \brief ajoute les arrets aux voyages présents dans le GTFS si l'heure du voyage appartient à l'intervalle de temps du GTFS
//! \brief Un arrêt est ajouté SSI son heure de départ est >= now1 et que son heure d'arrivée est < now2
//! \brief De plus, on enlève les voyages qui n'ont pas d'arrêts dans l'intervalle de temps du GTFS
//! \brief De plus, on enlève les stations qui n'ont pas d'arrets dans l'intervalle de temps du GTFS
//! \param[in] p_nomFichier: le nom du fichier contenant les arrets
//! \post assigne m_tousLesArretsPresents à true
//! \throws logic_error si un problème survient avec la lecture du fichier
void DonneesGTFS::ajouterArretsDesVoyagesDeLaDate(const std::string &p_nomFichier)
{
    ifstream arret_fichier(p_nomFichier);
    string arret;
    getline(arret_fichier, arret);

    try {
        while (getline(arret_fichier, arret)) {
            vector<string> informationArret= string_to_vector(arret, ',');

            Heure heure_arriver(stoi(informationArret[1].substr(0, 2)),
                                stoi(informationArret[1].substr(3,2)),
                                stoi(informationArret[1].substr(6,2)));

            Heure heure_depart(stoi(informationArret[2].substr(0,2)),
                      stoi(informationArret[2].substr(3,2)),
                      stoi(informationArret[2].substr(6,2)));

            if(getTempsDebut() <= heure_depart && heure_arriver < getTempsFin() && m_voyages.find(informationArret[0]) != m_voyages.end()){

                Arret::Ptr arret_ptr = make_shared<Arret>(informationArret[3],
                                                           heure_arriver, heure_depart,
                                                           stoi(informationArret[4]), informationArret[0]);
                m_nbArrets++;

                m_voyages.at(informationArret[0]).ajouterArret(arret_ptr);

                m_stations.at(informationArret[3]).addArret(arret_ptr);
            }
        }
    }
    catch (...){
        throw logic_error("Le fichier " + p_nomFichier + " n'a pas été lu correctement");
    }

    for (auto it = m_voyages.cbegin(); it != m_voyages.cend();)
    {
        if (it->second.getNbArrets()==0)
            m_voyages.erase(it++);
        else
            ++it;
    }

    for (auto it = m_stations.cbegin(); it != m_stations.cend();)
    {
        if (it->second.getNbArrets()==0)
            m_stations.erase(it++);
        else
            ++it;
    }

    m_tousLesArretsPresents = true;
}



