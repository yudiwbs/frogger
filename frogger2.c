#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <pthread.h> 
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

/*
  Frogger Sederhana.
  yudi@upi.edu, untuk kuliah sistem operasi (topik multithread)
  cara meng-compile dan menjalankan: 
  
  gcc -pthread frogger.c -o frogger.o -lncurses
  ./frogger.o 
  
  referensi singkat tentang lib curses: http://www6.uniovi.es/cscene/CS3/CS3-08.html
*/

bool isStop = false;  
bool isPause = false;
WINDOW * mainwin;
int max_y, max_x;
pthread_mutex_t lock;

int pRand(int min, int max)
{
    max -= 1;
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

//------------------------------------------
/*  init ncurses  */
void initCurses() {
  if ( (mainwin = initscr()) == NULL ) {
        fprintf(stderr, "Error initialising ncurses.\n");
        exit(EXIT_FAILURE);
  }
  clear();
  noecho();
  curs_set(0);
  cbreak();
  if ((curs_set(0)) == 1) {
	fprintf(stderr, "Error menyembuniykanb cursor \n");
    exit(EXIT_FAILURE);
  }	
  keypad(mainwin, TRUE);
  box( mainwin, ACS_VLINE, ACS_HLINE );
  getmaxyx(mainwin, max_y, max_x);
  refresh();
}

void terminateCurses() {
   curs_set(1);
   clear();
   refresh();
   resetty();	
   endwin();
}
//pesan untuk user di kiri bawah
void printNotif(char* msg) {
   //bersihkan	
   mvprintw(max_y-2, 4, "                                                                           ");
   //tulis pesan
   mvprintw(max_y-2, 4, msg);
   refresh();
}	

// ========================  frog
//data mengenai frog  
typedef struct
{ 
	int  x;
    int  y;
} Kodok;

Kodok kodok;  //user

//aksi user
#define ATAS 1
#define BAWAH 2
#define KIRI 3
#define KANAN 4
#define NONE 0

void frogAction(int aksi) {
	if (isPause) {
       return; //tdk melakukan apa2 jika dipause
 	}	
	pthread_mutex_lock(&lock);
	move(kodok.y,kodok.x);
    addch(' '); //clear
	if (aksi == ATAS) {
		kodok.y = kodok.y-1 ; 
		if (kodok.y < 2) { // finish menyebrang
			printNotif("Selamat anda berhasil \\(^o^)/ Main lagi? (y/t)");
			isPause = true;
		}	
	} else if (aksi==BAWAH) {
		kodok.y = kodok.y+1;
		if (kodok.y > max_y-5) {  // mentok ke bawah
			kodok.y = max_y-5;
		} 
    } else if (aksi==KANAN) {
        kodok.x = kodok.x+1;  
		if (kodok.x > max_x-3) {  // mentok ke kanan
			kodok.x = max_x-3;
		} 
  	} else if (aksi==KIRI) {
		kodok.x = kodok.x-1;
		if (kodok.x < 3) {  // mentok ke kiri
			kodok.x = 3;
		} 
    } 	
    //gambar di tempat baru	
	move(kodok.y,kodok.x);
    addch('o');
    refresh();	
	pthread_mutex_unlock(&lock);
}	

//init kodok
//dipanggil saat game mulai
void frogInit() {
   move(kodok.y,kodok.x);
   addch(' '); //clear
   kodok.x = (int) (max_x / 2);
   kodok.y = max_y-5;   
   frogAction(NONE);
   printNotif("Bantu kodok menyebrang jalan. Panah: gerakan kodok; x: Keluar");
}

// ================= MOBIL
typedef struct
{
	//int arah;   
	int  x;
    int  y;
	int delay;  //dalam milidetik, semakin besar, semakin lambat kecepatan mobil
	char karakter;
} Mobil;

#define JUMMOBIL 5
Mobil arrMobil[JUMMOBIL];

//init mobil-mobil yang akan ditampilkan
void mobilInit() {
	for (int i=0; i<=JUMMOBIL; i++) {
		arrMobil[i].x = pRand(1,3);
		arrMobil[i].y = (i+1)*2; //ada jarak
		
		//kalau ukuran window tdk cukup
		if (arrMobil[i].y > max_y-5) {
			fprintf(stderr, "window terlalu kecil, perbesar \n"); 
			terminateCurses();
			exit(0);
		}	
	}
	//atur karakter dan kecepatan mobil
    arrMobil[0].karakter='>';  
	arrMobil[0].delay = 10;
	arrMobil[1].karakter=']';	
	arrMobil[1].delay = 15;
	arrMobil[2].karakter='=';	
	arrMobil[2].delay = 75;
	arrMobil[3].karakter=']';	
	arrMobil[3].delay = 150;
	arrMobil[4].karakter=']';	
	arrMobil[4].delay = 90;
}

//gambar untuk satu mobil 	
void *drawMobil(void *i) { 
	//ambil mobil yg terkait di array
	int idxMobil = *((int *) i);
    free(i);
	Mobil mobil; 
	mobil = arrMobil[idxMobil];
	
	//untuk delay
	struct timespec tim, tim2;
	tim.tv_sec  = 0;  
    tim.tv_nsec = mobil.delay * 1000000L; //dalam nano
	
	//gerakan mobil 	
	while (!isStop) {
		pthread_mutex_lock(&lock); 
		move(mobil.y,mobil.x);
		addch(' ');  //clear
		
		//jika sampai ke ujung, reset ke posisi awal 
		if (mobil.x > max_x-5) {
			mobil.x = pRand(1,3); //mulai di posisi random 
	    }	
		mobil.x++;  
		move(mobil.y,mobil.x);
		addch(mobil.karakter); //draw
		refresh();
		//cek tabrakan
		if ((mobil.x == kodok.x) && (mobil.y == kodok.y)) {
			printNotif("Kodok tertabrak (x_x) Main lagi? (y/t)");
			isPause = true;
		}	
		pthread_mutex_unlock(&lock); 
		nanosleep(&tim , &tim2);
	}
}

// ===================================== END MOBIL
void *inputUser(void *vargp) 
{ 
    while (!isStop) {
		int ch = wgetch(mainwin);
		if(ch == KEY_LEFT) {
		  frogAction(KIRI);
		}  
	    else if (ch == KEY_RIGHT) {
		  frogAction(KANAN);	
		} 
	    else if (ch == KEY_UP) {
		  frogAction(ATAS);
		}  
		else if (ch == KEY_DOWN) {
		   frogAction(BAWAH);	  
		}  
		else if ((ch == 'x') || (ch == 'x'))  {  
		   isStop = true;
		}  
		else if ((isPause) && ( (ch=='Y') || (ch=='y'))) {
		   isPause = false;
		   frogInit();	 //mulai lagi
		}
		else if ((isPause) && ( (ch=='T') || (ch=='t'))) {
		   isStop = true;	 
		}		
		
	}
    return NULL; 
}

int main() {
  pthread_t tid_input;  //input dari user
  pthread_t arrThreadIdMobil[JUMMOBIL]; //mobil
   
  initCurses();
  mobilInit();
  frogInit();
  
  isStop = false;
  isPause = false;
  //menerima input user
  pthread_create(&tid_input, NULL, inputUser, NULL); 
  
  //thread untuk pergerakan mobil-mobil 
  for (int i=0;i<=JUMMOBIL-1;i++) {
	  int *arg = malloc(sizeof(*arg));
	  *arg = i;
	  pthread_create(&arrThreadIdMobil[i], NULL, drawMobil, arg); //index arrMobil jadi parameter
  }
  
  //selesai	
  pthread_join(tid_input, NULL); 
  for (int i=0;i<=JUMMOBIL-1;i++) {
	   pthread_join(arrThreadIdMobil[i], NULL);  
  }
  terminateCurses();
}