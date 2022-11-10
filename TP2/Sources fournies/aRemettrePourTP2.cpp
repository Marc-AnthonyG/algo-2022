//
// Created by Mario Marchand on 16-12-30.
//

#include "ReseauGTFS.h"

using namespace std;

//! \brief ajout des arcs dus aux voyages
//! \brief insère les arrêts (associés aux sommets) dans m_arretDuSommet et m_sommetDeArret
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsVoyages(const DonneesGTFS & p_gtfs)
{
    try {
        for (auto &voyage: p_gtfs.getVoyages()) {
            auto arret = voyage.second.getArrets().begin();
            auto arretSuivant = voyage.second.getArrets().begin();
            m_arretDuSommet.push_back(*arretSuivant);
            m_sommetDeArret.insert({*arretSuivant, m_arretDuSommet.size() - 1});
            ++arretSuivant;

            while (arretSuivant != voyage.second.getArrets().end()) {
                m_arretDuSommet.push_back(*arretSuivant);
                m_sommetDeArret.insert({*arretSuivant, m_arretDuSommet.size() - 1});
                this->m_leGraphe.ajouterArc(m_sommetDeArret.at(*arret),
                                            m_sommetDeArret.at(*arretSuivant),
                                            arretSuivant->get()->getHeureArrivee()-arret->get()->getHeureArrivee());
                arretSuivant++;
                arret++;
            }
        }
    }
    catch (...){
        throw logic_error("ajouterArcsVoyages");
    }
}


//! \brief ajouts des arcs dus aux transferts entre stations
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe

void ReseauGTFS::ajouterArcsTransferts(const DonneesGTFS & p_gtfs)
{
    try {
        for (auto &transfert : p_gtfs.getTransferts()) {

            for (auto &arretOrigine : p_gtfs.getStations().at(get<0>(transfert)).getArrets()) {
                set<string> ligneDejaUtilisee = {};
                ligneDejaUtilisee.insert(p_gtfs.getLignes().find(p_gtfs.getVoyages().find(arretOrigine.second->getVoyageId())->second.getLigne())->second.getNumero());

                auto arretDestinationPossible = p_gtfs.getStations().at(get<1>(transfert)).getArrets().lower_bound(arretOrigine.first.add_secondes(get<2>(transfert)));

                while (arretDestinationPossible != p_gtfs.getStations().at(get<1>(transfert)).getArrets().end())
                {
                    string ligneDestination = p_gtfs.getLignes().find(p_gtfs.getVoyages().find(arretDestinationPossible->second->getVoyageId())->second.getLigne())->second.getNumero();

                    //la ligne de l'arrêt est différent de l'arrêt d'origine ou d'un arrêt déjà ajouter pour l'arrêt d'origine
                    if(ligneDejaUtilisee.find(ligneDestination) == ligneDejaUtilisee.end())
                    {
                        ligneDejaUtilisee.insert(ligneDestination);
                        this->m_leGraphe.ajouterArc(m_sommetDeArret.at(arretOrigine.second),
                                                        m_sommetDeArret.at(arretDestinationPossible->second),
                                                    arretDestinationPossible->first-arretOrigine.first);
                    }
                    arretDestinationPossible++;
                }
            }
        }
    }
    catch (logic_error){
        throw logic_error("Il y a une erreur");
    }
}

//! \brief ajouts des arcs d'une station à elle-même pour les stations qui ne sont pas dans DonneesGTFS::m_stationsDeTransfert
//! \throws logic_error si une incohérence est détecté lors de cette étape de construction du graphe
void ReseauGTFS::ajouterArcsAttente(const DonneesGTFS & p_gtfs)
{
    try {
        for (auto &station : p_gtfs.getStations()) {
            if (p_gtfs.getStationsDeTransfert().find(station.first) == p_gtfs.getStationsDeTransfert().end()) {
                for (auto &arretOrigine : station.second.getArrets()) {
                    auto arretDestinationPossible = station.second.getArrets().lower_bound(arretOrigine.first.add_secondes(this->delaisMinArcsAttente));
                    set<string> ligneDejaUtilisee = {};
                    ligneDejaUtilisee.insert(p_gtfs.getLignes().find(p_gtfs.getVoyages().find(arretOrigine.second->getVoyageId())->second.getLigne())->second.getNumero());

                    while (arretDestinationPossible != station.second.getArrets().end()) {
                        string ligneDestination = p_gtfs.getLignes().find(p_gtfs.getVoyages().find(arretDestinationPossible->second->getVoyageId())->second.getLigne())->second.getNumero();
                        if(ligneDejaUtilisee.find(ligneDestination) == ligneDejaUtilisee.end()){

                            ligneDejaUtilisee.insert(ligneDestination);
                            this->m_leGraphe.ajouterArc(m_sommetDeArret.at(arretOrigine.second),
                                                        m_sommetDeArret.at(arretDestinationPossible->second),
                                                        arretOrigine.second->getHeureArrivee() -
                                                                arretDestinationPossible->first);
                        }
                        arretDestinationPossible++;
                    }
                }
            }
        }
    }
    catch (...){
        throw logic_error("ajouterArcsAttente");
    }
}


//! \brief ajoute des arcs au réseau GTFS à partir des données GTFS
//! \brief Il s'agit des arcs allant du point origine vers une station si celle-ci est accessible à pieds et des arcs allant d'une station vers le point destination
//! \param[in] p_gtfs: un objet DonneesGTFS
//! \param[in] p_pointOrigine: les coordonnées GPS du point origine
//! \param[in] p_pointDestination: les coordonnées GPS du point destination
//! \throws logic_error si une incohérence est détecté lors de la construction du graphe
//! \post constuit un réseau GTFS représenté par un graphe orienté pondéré avec poids non négatifs
//! \post assigne la variable m_origine_dest_ajoute à true (car les points orignine et destination font parti du graphe)
//! \post insère dans m_sommetsVersDestination les numéros de sommets connctés au point destination
void ReseauGTFS::ajouterArcsOrigineDestination(const DonneesGTFS &p_gtfs, const Coordonnees &p_pointOrigine,
                                               const Coordonnees &p_pointDestination)
{

    Arret::Ptr arret_ptr_origine = make_shared<Arret>(this->stationIdOrigine, Heure(), Heure(),0, "origine");
    m_arretDuSommet.push_back(arret_ptr_origine);
    m_sommetDeArret.insert({arret_ptr_origine, m_arretDuSommet.size() - 1});
    this->m_sommetOrigine = m_sommetDeArret.at(arret_ptr_origine);

    shared_ptr<Arret> arret_ptr_Destination = make_shared<Arret>(this->stationIdDestination, Heure(), Heure(),0, "destination");
    m_arretDuSommet.push_back(arret_ptr_Destination);
    m_sommetDeArret.insert({arret_ptr_Destination, m_arretDuSommet.size() - 1});
    this->m_sommetDestination = m_sommetDeArret.at(arret_ptr_Destination);

    this->m_leGraphe.resize(this->m_leGraphe.getNbArcs() + 2);

    cout << "it work" << endl;

    //DONE il faut ajouter les arcs qui sont avant la distance de marche max
    //DONE il doivent être possible d'atteindre avant le temps de l'autobus
    //DONE l'arc a comme poids le temps de marche plus le temps d'attente (autrement dit juste le temps d'attendre de ceux qui sont valides)
    //DONE l'arc a comme poids le temps de marche plus le temps d'attente (autrement dit juste le temps d'attendre de ceux qui sont valides)
    //TODO la lower_bound est mal setter on va avoir tout les arrêts


    for (auto &station:p_gtfs.getStations()){
        set<string> ligneDejaUtilisee = {};
        double distanceMarcheStation = sqrt(pow(station.second.getCoords().getLatitude()-p_pointOrigine.getLatitude(),2)+pow(station.second.getCoords().getLongitude()-p_pointOrigine.getLongitude(),2));
        if ( distanceMarcheStation <= this->distanceMaxMarche){
            auto arret = station.second.getArrets().lower_bound(Heure(0,0,ceil(distanceMarcheStation * this->vitesseDeMarche)));
            while (arret != station.second.getArrets().end()){
                if(ligneDejaUtilisee.find(p_gtfs.getVoyages().find(arret->second->getVoyageId())->second.getLigne())!=ligneDejaUtilisee.end()){
                    ligneDejaUtilisee.insert(p_gtfs.getVoyages().find(arret->second->getVoyageId())->second.getLigne());
                    try {
                        this->m_leGraphe.getPoids(m_sommetOrigine, m_sommetDeArret.at(arret->second));
                    }
                    catch (logic_error){
                        this->m_leGraphe.ajouterArc(m_sommetOrigine,
                                                    m_sommetDeArret.at(arret->second),
                                                    ceil(distanceMarcheStation * this->vitesseDeMarche));
                    }
                }
                arret++;
            }
        }
    }

    for (auto &station:p_gtfs.getStations()){
        double distanceMarcheStation = sqrt(pow(station.second.getCoords().getLatitude()-p_pointDestination.getLatitude(),2)+pow(station.second.getCoords().getLongitude()-p_pointDestination.getLongitude(),2));
        if ( distanceMarcheStation <= this->distanceMaxMarche){
            auto arret = station.second.getArrets().lower_bound(Heure(0,0,ceil(distanceMarcheStation * this->vitesseDeMarche)));
            while (arret != station.second.getArrets().end()){
                this->m_leGraphe.ajouterArc(m_sommetDeArret.at(arret->second),
                                            m_sommetDestination,
                                            ceil(distanceMarcheStation * this->vitesseDeMarche));
                arret++;
            }
        }
    }
    m_origine_dest_ajoute = true;
}

//! \brief Remet ReseauGTFS dans l'était qu'il était avant l'exécution de ReseauGTFS::ajouterArcsOrigineDestination()
//! \param[in] p_gtfs: un objet DonneesGTFS
//! \throws logic_error si une incohérence est détecté lors de la modification du graphe
//! \post Enlève de ReaseauGTFS tous les arcs allant du point source vers un arrêt de station et ceux allant d'un arrêt de station vers la destination
//! \post assigne la variable m_origine_dest_ajoute à false (les points orignine et destination sont enlevés du graphe)
//! \post enlève les données de m_sommetsVersDestination
void ReseauGTFS::enleverArcsOrigineDestination()
{
    try {
    for(auto sommet : m_sommetsVersDestination){
        for (auto sommet2 : m_sommetsVersDestination) {
            try{
                m_leGraphe.enleverArc(sommet2, sommet);
            }
            catch (logic_error){}

            try{
                m_leGraphe.enleverArc(sommet, sommet2);
            }
            catch (logic_error){}
        }
    }

    this->m_leGraphe.resize(m_sommetsVersDestination.size()-2);
    m_sommetDeArret.clear();
    m_arretDuSommet.clear();
    m_nbArcsStationsVersDestination = 0;
    m_nbArcsOrigineVersStations = 0;
    }
    catch (...){
        throw logic_error("EnleverArcsOrigineDestination");
    }
}


