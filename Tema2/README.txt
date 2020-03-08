Nume - Tomoescu Iulia-Cristina
Grupa - 324CC


 *Subscriber:

    Pentru aceasta tema am folosit socketi TCP in subscriber pentru a face
comunicare cu serverul. Pentru inceput am realizat conexiunea cu serverul.
Dupa ce fac conectarea cu serverul, trimit imediat un mesaj prin care
specific id-ul clientului. Pentru fiecare comanda primita de la tastatura,
o parsez, si in cazul in care nu sunt corecte, nu voi trimite mesajul catre
server. Pentru topic-uri mai lungi de 50 de caractere, nu voi trimite comanda
catre server, deoarece nu va exista nici un topic de dimensiune mai mare de 50.

 *Server:

    In server am realizat conexiunile cu clientii TCP si UDP. Cand primesc un
mesaj de la un client UDP, il parsez si in cazul in care am clienti TCP abonati
la topicul respectiv, le trimit mesajul imediat. In cazul in care un client TCP
exte deconectat, dar abonat cu store anf foreward pentru un topic, voi memora
acel mesaj pentru clientul curent, pentru a-l putea trimite cand se reconecteaza.
    Pentru fiecare subscribe, daca nu exista deja acel topic, creez topicul
respectiv si adaug clientul. Ulterior daca mai sunt si alti clienti care vor sa
se conecteze la acel topic, ii adaug in lista. La un unsubscribe, scot
clientul din lista topicului.
