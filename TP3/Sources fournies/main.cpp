//
// Produced by Mario on Dec 2016.
//

#include <iostream>
#include <random>

#include "DonneesGTFS.h"
#include "ReseauGTFS.h"

using namespace std;

int main()
{
    const std::string chemin_dossier = "../RTC-1aout-25nov";
    Date today(2022, 8, 3);
    Heure now1(7, 30, 0);
//  Date today; //Le constructeur par défaut initialise la date à aujourd'hui
//  Heure now1; //Le constructeur par défaut initialise l'heure à maintenant
    Heure now2 = now1.add_secondes(72000); //on désire obtenir tous les arrêts du reste de la journée

    clock_t begin = clock();
    DonneesGTFS donnees_rtc(today, now1, now2);
    donnees_rtc.ajouterLignes(chemin_dossier + "/routes.txt");
    cout << "Nombre de lignes = " << donnees_rtc.getNbLignes() << endl;
    donnees_rtc.ajouterStations(chemin_dossier + "/stops.txt");
    cout << "Nombre de stations initiales = " << donnees_rtc.getNbStations() << endl;
    donnees_rtc.ajouterServices(chemin_dossier + "/calendar_dates.txt");
    size_t nb_services = donnees_rtc.getNbServices();
    cout << "Nombre de services = " << nb_services << endl;
    if (nb_services == 0) throw logic_error("main(): On doit avoir nb_services > 0 pour continuer");
    donnees_rtc.ajouterVoyagesDeLaDate(chemin_dossier + "/trips.txt");
    donnees_rtc.ajouterArretsDesVoyagesDeLaDate(chemin_dossier + "/stop_times.txt");
    donnees_rtc.ajouterTransferts(chemin_dossier + "/transfers.txt");
    clock_t end = clock();
    cout << "Chargement des données effectué en " << double(end - begin) / CLOCKS_PER_SEC << " secondes" << endl;
    cout << "Nombre de stations ayant au moins 1 arrêt = " << donnees_rtc.getNbStations() << endl;
    cout << "Nombre de transferts = " << donnees_rtc.getNbTransferts() << endl;
    cout << "Nombres de voyages = " << donnees_rtc.getNbVoyages() << endl;
    cout << "Nombre d'arrêts = " << donnees_rtc.getNbArrets() << endl;
    begin = clock();
    ReseauGTFS reseau_rtc(donnees_rtc);
    end = clock();
    cout << "Le nombre d'arcs (sans le point origine et destination) est = " << reseau_rtc.getNbArcs() << endl;
    cout << "Graphe (sans le point source et destination) a été produit en " << double(end - begin) / CLOCKS_PER_SEC
         << " secondes" << endl << endl;

    cout << "==========================================" << endl;
    cout << "           début de la simulation         " << endl;
    cout << "==========================================" << endl << endl;

    //placement des stations_id dans un vector pour une meilleure sélection aléatoire
    vector<std::string> station_ids;
    const auto &stations = donnees_rtc.getStations();
    for (const auto &station : stations)
    {
        station_ids.push_back(station.first);
    }

    std::default_random_engine generator;
    std::uniform_int_distribution<unsigned int> distribution(0, (unsigned int) (station_ids.size() -
                                                                                1)); //range of numbers
    for (int i = 1; i <= 653; ++i) //réchauffement du générateur de nombres aléatoires
    {
        distribution(generator);
    }

    bool afficherItineraire = true;
    const unsigned int nbDeTests = 200; //nombre de tests à effectuer
    long moy_tempsExecution = 0;

    unsigned int nbDeTestsComptabilises = 0;
    //on comptabilise un test seulement si la destination est atteignable et différente de l'origine
    for (unsigned int i = 0; i < nbDeTests; ++i)
    {
        cout << endl << "Test numéro " << i << endl;
        unsigned int temp = distribution(generator);  // generates number in specified range above
        std::string stationIdOrigine = station_ids.at(temp);
        std::string stationIdDestination = stationIdOrigine;
        Coordonnees pointOrigine = stations.at(stationIdOrigine).getCoords();
        Coordonnees pointDestination = pointOrigine;

        while (stationIdOrigine == stationIdDestination ||
               pointOrigine - pointDestination <= 2.1 * reseau_rtc.getDistMaxMarche())
        {
            temp = distribution(generator);
            stationIdDestination = station_ids.at(temp);
            pointDestination = stations.at(stationIdDestination).getCoords();
        }

        cout << "station du point origine = " << stations.at(stationIdOrigine) << endl;
        cout << "station du point destination = " << stations.at(stationIdDestination) << endl;
        cout << "distance = " << pointOrigine - pointDestination << " kilomètres" << endl;

        reseau_rtc.ajouterArcsOrigineDestination(donnees_rtc, pointOrigine, pointDestination);

        long tempsExecution(0);
        unsigned int tempsDuTrajet = reseau_rtc.itineraire(donnees_rtc, afficherItineraire, tempsExecution);
        if (tempsDuTrajet == numeric_limits<unsigned int>::max())
        {
            cout << "impossible d'atteindre la destination. On passe au test suivant." << endl;
        }
        else if (tempsDuTrajet == 0)
        {
            cout << "Temps de trajet nul. On passe au test suivant." << endl;
        }
        else
        {
            moy_tempsExecution += tempsExecution;
            ++nbDeTestsComptabilises;
            cout << "Temps d'exécution de l'algorithme de plus court chemin: " << tempsExecution
                 << " microsecondes" << endl;

        }
        reseau_rtc.enleverArcsOrigineDestination();

    }

    cout << endl << "La moyenne du temps d'exécution sur " << nbDeTests << " itinéraires est de "
         << (double)moy_tempsExecution / (double)nbDeTestsComptabilises << " microsecondes" << endl;

    return 0;
}


