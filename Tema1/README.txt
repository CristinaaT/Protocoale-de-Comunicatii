    Protocoale de comunicații - Tema1
    Protocol cu fereastră glisantă

    Student - Tomoescu Iulia-Cristina
    Grupa - 324CC

    Salutare!

    1. Organizare
  - În rezolvarea temei am ales să scriu în cele două fișiere inițiale send.c
și recv.c, iar pe lângă acestea am mai adăugat un fișier header window.h în
care am definit structura window_packet care va fi copiată în payload-ul unei
structuri msg precum și o funcție simplă care calculează o sumă de verificare.
  - Consider că implementarea mea este eficientă, deoarece calculez optim
mărimea ferestei glisante iar tipul de protocol de fereastră glisantă pe care
l-am implementat este de tipul selective-repeat, adică retrimit pachete doar la
nevoie și în mod optim.
  - O să punctez doar câteva aspecte care nu reies doar din citirea codului și
a comentariilor din acesta.

    2. Implementare
  - Întregul enunț al temei a fost implementat.
  - Pentru fiecare alocare de memorie, deschidere de fișier sau accesare de
vector am făcut verificările necesare.
  - Mai întâi m-am asigurat că primul pachet ajunge în siguranță deoarece
conține numele fișierului și dimensiunea ferestrei (care va fi aceeași și
pentru sender și pentru receiver).
  - Pentru verificarea erorilor am făcut xor între toți octeții din payload și
am refăcut asta și în receiver.
  - Dacă în receiver se primesc pachete în altă ordine, acesta le stochează
în buffer, până când va avea nevoie de ele. Această verificare se face imediat
după ce a primit un pachet așteptat (adică verifică dacă următoarele pachete
așteptate se regăsesc în buffer).
  - Am testat tema local pe o arhitectură de 64 de biți cu LOSS, REORDER și
CORUPT având valorea 50 fiecare, iar mărimea fișierului de 100MB. Fișierul a
fost transmis cu succes.
  - O parte din timpul de execuție este folosit și pentru alocarea /
dezalocarea de memorie.
