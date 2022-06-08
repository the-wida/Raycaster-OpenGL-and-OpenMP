//BIBLIOTEKE ZA OPENGL
#include<GL/glut.h>
#include<GL/gl.h>
#include<GL/glu.h>
//STANDARDNE BIBLIOTEKE
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>
//OMP BIBLIOTEKA
#include<omp.h>
//ZA SYSTEM KOMANDE, VISUAL STUDIO DODAJE SAM
#include<windows.h>

//DEFINICIJA VRIJEDNOSTI KOJE CEMO KASNIJE KORISTITI
#define PI 3.141592
#define WIDTH 1024//SIRINA EKRANA U PIKSELIMA
#define HEIGHT 512//VISINA EKRANA U PIKSELIMA
#define MAP_WIDTH 8//SIRINA SVIJETA U CELIJAMA
#define MAP_HEIGHT 8//VISINA SVIJETA U CELIJAMA
#define MAP_BLOCK 64//SIRINA I VISINA JEDNE CELIJE U PIKSELIMA, DAKLE UKUPNA SIRINA, ODNOSNO VISINA SVIJETA JE -OVO- PUTA MAP_WIDTH, ODNOSNO MAP_HEIGHT
#define MAP_LINE 1//POLUDEBLJINA LINIJA KOJE OZNACAVAJU POZICIJE CELIJA (tj. GRANICE CELIJA)
#define EXTRA_OFFSET 20//PRAZNINA IZMEDJU LIJEVOG I DESNOG DIJELA EKRANA
#define OFFSET MAP_BLOCK*MAP_WIDTH+EXTRA_OFFSET//POZICIJA DESNOG DIJELA EKRANA
#define TOP_OFFSET 1 // ODVOJENOST DESNOG DIJELA EKRANA OD VRHA PROZORA
#define DOF 8// (DEPTH OF FIELD)-OZNACAVA BROJ PRETRAZIVANJA, DA NE POSTOJI-MAPA BEZ ZATVORENIH GRANICA BI UVELA PROCESOR U BESKONACNU PETLJU
#define DEGREE 0.0174533//JEDAN STEPEN U RADIJANIMA
#define FOV 60//SIRINA POLJA PREGLEDNOSTI KAMERE (FIELD OF VIEW)
#define SKIP_FRAMES 5//PRESKACEMO POCETNE FREJMOVE, JER NISU REPREZENTATIVNI ZA PERFORMANSE(VELIKE FLUKTUACIJE OD 0ms DO >100ms)
#define BENCHMARK_CYCLES 3 //KOLIKO PUTA PONOVITI BENCHMARK(DEFAULT)

//DEKLARACIJE VRIJEDNOSTI  //NAPOMENA: KADA KAZEM X,ILI Y UDALJENOST, GOVORIM O PROJEKCIJI UDALJENOSTI NA X, ODNOSNO Y OSU //NAPOMENA: Y OSA IDE ODOZGO NA DOLE
float posx, posy, posa, rayAngle, rayAngleInit;//POZICIJA I UGAO KAMERE, KAO I UGAO POJEDINOG ZRAKA IZ KAMERE, KAMERA SALJE JEDNU ZRAKU ZA SVAKU KOLONU PIKSELA KOJU VIDI=>VISE ZRAKA, VECA REZOLUCIJA (AKO JE FOV KONST)
float distx, disty;//KOLICINA KRETANJA KAMERE PO X I Y OSI KADA JE POMIJERAMO NAPRIJED I NAZAD
int i,j,cntr,rayCounter;//BROJACI
int cellx,celly,cell,cellpos;//PROMJENJIVE KOJE SLUZE ZA TRANSLACIJU STVARNE POZICIJE U MAPI U REDNO MJESTO CELIJE U MATRICI map[](U OVOM SLUCAJU JE NIZ (TODO?)), cell JE VRIJEDNOST NA TOJ POZICIJI
int colorV,colorH,colorActual;//ZA BOJU ZIDA                                                      //^^^JEDNOSTAVNIJE RECENO: ODREDJUJU KOJOJ CELIJI PRIPADA NEKO MJESTO NA MAPI
FILE *rateLog;//GDJE UPISUJEMO LOG
char buffer[20];//POMOCNA PROMJENJIVA ZA UPISIVANJE U LOG
float tang;//POMOCNA PROMJENJIVA KOJA CUVA TANGENS UGLA IZMEDJU POJEDINOG ZRAKA IZ KAMERE I OSE X
float disth, distv, distActual;//UDALJENOST PRVOG VERTIKALNOG I HORIZONTALNOG ZIDA OD KAMERE, KAO I UDALJENOST NAJBLIZEG ZIDA OD KAMERE (ZID KOJI KAMERA U STVARI VIDI)
float rayxinit,rayyinit;//INICIJALNO MJESTO ZA PROVJERU KOLIZIJE ZRAKE SA ZIDOM (NARAVNO ZRAKA PRVO PRODJE KROZ ZIDOVE CELIJE U KOJOJ SE NALAZI KAMERA)
float initOffsetx,initOffsety;//POMOCNA VRIJEDNOST ZA RACUNANJE VRIJEDNOSTI IZNAD ^^^(UDALJENOST VRIJEDNOSTI ^^^ I X, ODNOSNO Y POZICIJE SAME KAMERE)
float y, x;//POMOCNA VRIJEDNOST ZA RACUNANJE VRIJEDNOSTI IZNAD ^^^(X, ODNOSNO Y UDALJENOST POZICIJE KAMERE I PRVOG MJESTA GDJE ZRAKA PREDJE IZ JEDNE CELIJE U DRUGU(TJ. NAIDJE NA MJESTO GDJE BI MOGAO BITI ZID))
float rayx,rayy;//KRAJNJA X,ODNOSNO Y POZICIJA ZRAKE, TJ. MJESTO U KOJEM ZAISTA UDARI U ZID
float posyV,posyH,posxV,posxH;//^^^ISTA STVAR, SAMO STAVLJENO U POMOCNE PROMJENJIVE ZA VERTIKALNE I HORIZONTALNE ZIDOVE
float offsetx,offsety;//POMJERAJ KRAJNJE POZICIJE ZRAKE, BICE OBJASNJENO U FUNKCIJI Rays()
float lineoffset = (WIDTH-(MAP_WIDTH*MAP_BLOCK))/FOV;// GDJE SA DESNE STRANE UPISUJEMO LINIJU KOJA PREDSTAVLJA DIO ZIDA
float currentoffset,lineHeight,fisheyeFix,rayDelta;//JOS NEKE PROMJENJIVE KOJE OMOGUCAVAJU CRTANJE S DESNE STRANE, OBJASNJENE KASNIJE
int hitVert;//DA LI SMO POGODILI VERTIKALNI ZID, KORISTI SE ZA SJENCENJE
float frameStart, frameEnd, frameTime, framesPerSecond;//PROMJENJIVE ZA MJERENJE PERFORMANSI
float totalFrameTime = 0,avgTime,longestFrame=0.00,shortestFrame=1000.00;//OPET ZA PERFORMANSE
int totalFrames = 0;//ISTO
int threadNumber,threadNumberMax,threadNumberActual;//ZA FUNKCIONISANJE OPENMP-A
int map[] = {//MATRICA(NIZ) KOJA DEFINISE SVIJET, TJ MAPU. >0 SU POZICIJE ZIDOVA, 0 JE PRAZAN PROSTOR(DRUGI BROJEVI SE KORISTE ZA ZIDOVE RAZLICITE BOJE)
1,1,1,1,1,4,1,1,
1,0,2,0,0,0,0,1,
4,0,0,0,0,0,0,1,
4,0,2,0,0,0,1,1,
4,0,2,0,0,0,1,1,
4,0,0,0,3,2,0,4,
1,5,0,0,0,0,0,1,
1,1,3,3,3,1,1,1
};
int textu_en=0,textu[]={1,0,1,0,1,0,1,0},textusize;float pixelsize;//ZA TEKSTURE, IGNORISATI
int bmark_rs,bmark_en,moveCurrent=0,moveCycle=0, cycles=0;int moves[] = {0,1,0,0,0,1,1,0,0,0,0,2,2,2,0,0,0,4,0,0,0,4,0,3,0,0,0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,0,0,0,0,0,0,0,0,0,0,4,0,0,0,4,4,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0};//ZA BENCHMARK
float heights[FOV]; int colors[FOV], verts[FOV];//NIZOVI KOJI NAM OMOGUCAVAJU DA RENDEROVANJE IZMJESTIMO IZ OPENMP PETLJE
float rayxBegin, rayyBegin, rayxEnd, rayyEnd;//PROMJENJIVE KOJE OMOGUCAVAJU DA PRIKAZ VIDNOG POLJA IZMJESTIMO IZ OPENMP PETLJE
//ZA NAJBOLJE RAZUMIJEVANJE PROGRAMA KRENUTI OD FUNKCIJE main() KOJA JE NA KRAJU vvv

//KORISNIK KONTROLISE BROJ NITI
void getParams()
{
	omp_set_dynamic(0);
	threadNumberMax = omp_get_max_threads();															//
	do																									//
	{																									//	
		system("cls");																					//MOGUCE JE UNJETI >MAX NITI, ALI NIJE DOBRA PRAKSA
		printf("Input #of threads... Max:%d (or 0 for max) ", threadNumberMax);	                        //
		scanf("%d", &threadNumber);																		//
	}																									//
	while(threadNumber>threadNumberMax || threadNumber<-1);					    						//
	if (threadNumber==0)																				//UKOLIKO UNESEMO 0 STAVLJA SE NA MAX BROJ
	{
		threadNumber=threadNumberMax;	
	}
	if (threadNumber==-1)
	{
		threadNumber=1;
		textu_en = 1;	
	}
	do																						             //DA LI IZVRSAVAMO BENCHMARK			
	{																									
		system("cls");																					
		printf("Enable benchmark? (1/0):");	                        
		scanf("%d", &bmark_en);																		
	}																									
	while(bmark_en!=1 && bmark_en !=0);
	if(bmark_en ==1)
	{
		do																						         //PRIHVATANJE BROJA CIKLUSA BENCHMARKA,MORA NENEGATIVNO			
		{																									
			system("cls");																					
			printf("Benchmark enabled...\nInput # of cycles(or 0 for default): ");	                        
			scanf("%d", &cycles);																		
		}																									
		while(cycles<0);
		if(cycles==0) cycles = BENCHMARK_CYCLES;
		if(cycles>1)
		{
			do																						     //DA LI RESETUJEMO POZICIJU IZMEDJU CIKLUSA			
			{																									
				system("cls");																					
				printf("Benchmark enabled...\nReset position between cycles?(1/0): ");	                        
				scanf("%d", &bmark_rs);																		
			}																									
			while(bmark_rs!=1 && bmark_rs !=0);
		}
	}
}

//PISANJE U LOG
void writeToLog()
{
		rateLog=fopen("log.txt","a");
		time_t t = time(NULL);				//
		struct tm tm = *localtime(&t);		//FUNKCIJE SISTEMSKOG VREMENA
		fputs("Date: ",rateLog);						//STVARI KOJE SE UPISUJU SU OCIGLEDNE
		fputs(itoa(tm.tm_year+1900,buffer,10),rateLog);
		fputs(":",rateLog);
		fputs(itoa(tm.tm_mon+1,buffer,10),rateLog);
		fputs(":",rateLog);
		fputs(itoa(tm.tm_mday,buffer,10),rateLog);
		fputs(" Time: ",rateLog);
		fputs(itoa(tm.tm_hour,buffer,10),rateLog);
		fputs(".",rateLog);
		fputs(itoa(tm.tm_min,buffer,10),rateLog);
		fputs(".",rateLog);
		fputs(itoa(tm.tm_sec,buffer,10),rateLog);
		fputs(" #of Threads: ",rateLog);
		fputs(itoa(threadNumberActual,buffer,10),rateLog);
		//fputs(" Depth of Field: ",rateLog);
		//fputs(itoa(DOF,buffer,10),rateLog);
		fputs(" Shortest frame: ",rateLog);
		fputs(itoa(shortestFrame,buffer,10),rateLog);
		fputs("ms Longest frame: ",rateLog);
		fputs(itoa(longestFrame,buffer,10),rateLog);
		fputs("ms Average frame: ",rateLog);
		fputs(gcvt((double)avgTime,6,buffer),rateLog);
		fputs("ms Total frames: ",rateLog);
		fputs(itoa(totalFrames,buffer,10),rateLog);
		fputs(" Total frametime: ",rateLog);
		fputs(itoa(glutGet(GLUT_ELAPSED_TIME),buffer,10),rateLog);
		fputs(" Benchmark cycles: ",rateLog);
		fputs(itoa(cycles,buffer,10),rateLog);
		fputs(" Benchmark reset: ",rateLog);
		if(bmark_rs == 1) fputs(" Yes",rateLog);
		else fputs(" No",rateLog);
		fputs("\n\n",rateLog);
		fclose(rateLog);
}

//FUNKCIJA ZA IZVRSENJE BENCHMARK-A
void Benchmark()//moves[] JE NIZ KOMANDI, moveCurrent JE POZICIJA U TOM NIZU, moveCycle JE CIKLUS U KOME SE TRENUTNO NALAZIMO
{ 																																			
	switch (moves[moveCurrent])// ZA KOMANDE JEDAN DO CETIRI KOD JE ISTI KAO U FUNKCIJI Keyboard(), OBJASNJENJA SU TAMO
 	{
 		case 0:				   //PRAZNA KOMANDA											
 			 moveCurrent++;
			 break;
 		case 1:
 			moveCurrent++;
		 	cellx = (int)(posx+distx)/MAP_BLOCK;
	 		celly = (int)(posy+disty)/MAP_BLOCK;
	 		cellpos = celly*MAP_WIDTH+cellx;	 
	 		cell = map[cellpos];				 
	 		if(cell==0)						 
	 		{									
	 			posy+=disty;						 
		 		posx+=distx;						 
    		}																	
 			break;
 		case 2:
 			moveCurrent++;
 			posa -=0.1;
	 		if(posa<=0)
	  		{
	  			posa=2*PI;
	  		}
	 		distx = cos(posa)*5;
	  		disty = sin(posa)*5;
 			break;
 		case 3:
 			moveCurrent++;
 			posa +=0.1;
	  		if (posa>=2*PI)
	  		{
	  			posa=0;
	  		}
	  		distx = cos(posa)*5;
	  		disty = sin(posa)*5;
 			break;
 		case 4:
 			moveCurrent++;
 			cellx = (int)(posx-distx)/MAP_BLOCK;
			celly = (int)(posy-disty)/MAP_BLOCK;
	 		cellpos = celly*MAP_WIDTH+cellx;
	 		cell = map[cellpos];
	 		if(cell==0)
	 		{
	 			posy-=disty;
	 			posx-=distx;
     		}
 			break;
 		case 5:
 			if(moveCycle+1<cycles)		//DA LI JE BENCHMARK PROSAO KROZ DEFINISAN BROJ CIKLUSA
 			{
 				moveCycle++;
 				moveCurrent=0;
 				if(bmark_rs ==1)
 				{
 					posx=300;
					posy=300;
					posa=3*PI/2;
					distx = cos(posa)*5;
					disty = sin(posa)*5;
 				}
 			}
 			else								//AKO JESTE ZATVORITI PROGRAM NA ISTI NACIN KAO DA SMO PRITISNULI 'q', POGLEDATI FUNKCIJU Keyboard()
 			{
 				avgTime = totalFrameTime/(float)totalFrames;
				framesPerSecond = 1.0000/avgTime * 1000;
				printf("Average frame time=%f\n",avgTime);
				printf("FPS: %f\n",framesPerSecond);
				printf("Shortest frame time=%f\n",shortestFrame);
				printf("Longest frame time=%f\n",longestFrame);
				printf("Total frames elapsed=%d",totalFrames);
  			    writeToLog();
	  	        glutDestroyWindow(1);
	  	        exit(0);
	    	}
 			break;
 }  
}

//CRTAMO POZADINU SA DESNE STRANE, PLAFON PLAV, POD ZELEN
void background()
{
		glColor3f(0,0.3,0.7);
	    glBegin(GL_QUADS);
	    glVertex2d(OFFSET-lineoffset/2,0);
	    glVertex2d(WIDTH-EXTRA_OFFSET+lineoffset/2,0);
	    glVertex2d(WIDTH-EXTRA_OFFSET+lineoffset/2,HEIGHT/2);
	    glVertex2d(OFFSET-lineoffset/2,HEIGHT/2);
	    glEnd();
	    glColor3f(0,0.7,0.3);
	    glBegin(GL_QUADS);
	    glVertex2d(OFFSET-lineoffset/2,HEIGHT/2);
	    glVertex2d(WIDTH-EXTRA_OFFSET+lineoffset/2,HEIGHT/2);
	    glVertex2d(WIDTH-EXTRA_OFFSET+lineoffset/2,HEIGHT);
	    glVertex2d(OFFSET-lineoffset/2,HEIGHT);
	    glEnd();
}

//FUNKCIJA KOJA CRTA ZRAKE
void rays()
{
    rayAngleInit = posa-(FOV/2)*DEGREE-DEGREE;//UGAO PRVE ZRAKE SA LIJEVE STRANE
	omp_set_num_threads(threadNumber);		  //POSTAVI BROJ NITI ZA SUBSEKVENTNI PARALELNI DIO
	#pragma omp parallel private(rayAngle,disth,distv,distActual,cellx,celly,cellpos,cntr)//DEKLARACIJA PARALELNOG DIJELA + DEKLARACIJA PRIVATNIH PROMJENJIVIH
	{
	threadNumberActual = omp_get_num_threads();//POTVRDA DA ZAISTA IMAMO TAJ BROJ NITI
    #pragma omp for		//DEKLARACIJA PARALELNOG FOR REGIONA (EKVIVALENTNO DEKLARACIJI #PRAGMA OMP PARALLEL FOR)
	for (rayCounter = 0; rayCounter<FOV; rayCounter++)//GLAVNA FOR PETLJA, IDE S LIJEVA NA DESNO KROZ VIDNO POLJE 
	{
		rayAngle = rayAngleInit+DEGREE*rayCounter;	//UGAO ZRAKE ZA SVAKI POJEDINI DIO VIDNOG POLJA
		distv = 10000;//
		disth = 10000;//INICIJALNE UDALJENOSTI VERT. I HOR. ZIDOVA, BITNO JE JER JE MOGUCE DA UOPSTE NE POGODIMO VERT. ILI HOR. ZID, A MORAMO POREDITI OVE VRIJEDNOSTI, PA C VRACA NASUMICNE PODATKE
		if(rayAngle>2*PI)//ISTA STVAR KAO RANIJE, UGAO MOZE POBJECI IZ OKVIRA 0 DO 2*PI PA GA VRACAMO
		{
			rayAngle = rayAngle-2*PI; 
		}
		if(rayAngle<0)
		{
			rayAngle = 2*PI+rayAngle;
		}
		printf("DEBUG: ray#:%d Angle:%f RAD Distance %f units Step = %d\n",rayCounter,rayAngle,distActual,moveCurrent);//DEBUG INFORMACIJE U KONZOLI
		//POCETAK PROVJERE DA LI JE ZRAKA POGODILA HORIZONTALNI ZID
		if (rayAngle>PI)//OVO ZNACI DA ZRAKA GLEDA NAGORE, ZAPAMTI Y OSA IDE KA DOLE...OVO RADIMO JER JE MALO DRUGACIJA MATEMATIKA KADA GLEDAMO GORE, DOLE, LIJEVO I DESNO
		{//ZA GEOMETRIJU POGLEDATI FIGURU 1 ISPOD main()
			tang = tan(rayAngle);//TANGENS UGLA ZRAKE I X OSE
			rayyinit = ((int)(posy/MAP_BLOCK)*MAP_BLOCK)-0.0001;//GDJE PO PRVI PUT ZRAKA NAILAZI NA GRANICU IZMEDJU CELIJA, PRVI POTENCIJALNI ZID (-0.0001 ZA PRECIZNOST)
			y = posy-rayyinit;//Y JE RAZLIKA IZMEDJU Y POZICIJE KAMERE I POZICIJE NA Y OSI GDJE ZRAKA PO PRVI PUT NAILAZI NA GRANICU IZMEDJU CELIJA,JEDNOSTAVNIJE, UDALJENOST KAMERE OD VRHA CELIJE
			initOffsetx = (-y/tang);//UDALJENOST X KOORDINATE POZICIJE KAMERE I X KOORDINATE MJESTA GDJE ZRAKA UDARA U PRVU HOR. GRANICU CELIJA, DETALJNIJE OPISANO U FIGURI 1
			rayxinit = posx+initOffsetx;//STVARNA X KOORDINATA GDJE ZRAKA UDARA U HOR. GRANICU
			offsety = -MAP_BLOCK;//OBJASNJENO U FIGURI 1, UKRATKO RAZLIKA IZMEDJU Y KOORDINATA SVAKA DVA SUSJEDNA PRESJEKA ZRAKE I HOR. GRANICA
			offsetx = offsety/tang;//OBJASNJENO U FIGURI 1, UKRATKO RAZLIKA IZMEDJU X KOORDINATA SVAKA DVA SUSJEDNA PRESJEKA ZRAKE I HOR. GRANICA
			for (cntr = 0; cntr<DOF; cntr++)//PONAVLJAMO AKCIJU DODAVANJA OVIH POMJERAJA PO OSAMA, SVE DOK NE UDARIMO U NEKI ZID
			{ 
		    	rayy = rayyinit+(cntr*offsety);//
				rayx = rayxinit+(cntr*offsetx);//SLJEDECE POZICIJE GDJE SE ZRAKA SJECE SA HOR. GRANICOM CELIJA
				cellx = (int)(rayx)/MAP_BLOCK;
	        	celly = (int)(rayy)/MAP_BLOCK;
	        	cellpos = celly*MAP_WIDTH+cellx;
	        	if (cellpos<MAP_WIDTH*MAP_HEIGHT && cellpos>=0)//DA LI TIM POZICIJAMA ODGOVARA CELIJA KOJA SADRZI ZID?I PRIJE SVEGA DA LI SU TE POZICIJE VALIDNE ZA DATU MATRICU?
	        	{
	        		cell = map[cellpos];
	        		if(cell!=0)
	        		{
	        			cntr = DOF+1;
	        			disth = sqrt(pow((rayy-posy),2)+pow((rayx-posx),2));//AKO DA, ZAVRSI PRETRAGU I IZRACUNAJ UDALJENOST DO DATOG ZIDA PITAGORINOM TEOREMOM
	        			colorH = cell;
	        		}														//AKO NE, PETLJA SE NASTAVLJA
	        	}
			}
		}
		if(rayAngle<PI)//GLEDAMO DOLE
		{//MATEMATIKA JE ISTA, OZNACAVAM RAZLIKE
			tang = tan(rayAngle);
			rayyinit = ((int)(posy/MAP_BLOCK)*MAP_BLOCK)+MAP_BLOCK;//SADA JE ZID ISPOD, DAKLE ZA JEDNU CELIJU POMJEREN UNAPRIJED, STOGA +MAP_BLOCK
			y = rayyinit-posy;//OBRNUT SMIJER OPERATORA, DA BI REZULTAT BIO POZITIVAN, NIJE NEOPHODNO (1)
			initOffsetx = (y/tang);//ZBOG (1) OVDE NEMA MINUSA
			rayxinit = posx+initOffsetx;
			offsety = MAP_BLOCK;//IDEMO NADOLE, PA NEMA MINUSA
			offsetx = offsety/tang;
			for (cntr = 0; cntr<DOF; cntr++)
			{ 
		    	rayy = rayyinit+(cntr*offsety);
				rayx = rayxinit+(cntr*offsetx);
				cellx = (int)(rayx)/MAP_BLOCK;
	        	celly = (int)(rayy)/MAP_BLOCK;
	        	cellpos = celly*MAP_WIDTH+cellx;
	        	if (cellpos<MAP_WIDTH*MAP_HEIGHT && cellpos>=0)
	        	{
	        		cell = map[cellpos];
	        		if(cell!=0)
	        		{
	        			cntr = DOF+1;
	        			disth = sqrt(pow((rayy-posy),2)+pow((rayx-posx),2));
	        			colorH = cell;
	        		}
	        	}
			}
		}
		posyH = rayy;//
		posxH = rayx;//POZICIJE GDJE JE ZRAKA POGODILA ZID U HORIZONTALNOJ PRETRAZI
		if (rayAngle==0 || rayAngle==PI)//HORIZONTALNA PRETRAGA NEMA SMISLA AKO GLEDAMO DIREKTNO LIJEVO ILI DIREKTNO DESNO
		{
			disth = 10000;
		}
	//KRAJ PROVJERE DA LI JE ZRAKA POGODILA HORIZONTALNI ZID
	//POCETAK PROVJERE DA LI JE ZRAKA POGODILA VERTIKALNI ZID
		if (rayAngle>PI/2 && rayAngle<3*PI/2)//GLEDAMO LIJEVO
		{
			tang = tan(rayAngle-(PI)/2);//TAN (THETA) = TAN (THETA-PI/2),,,TRIG IDENTITET
			rayxinit = ((int)(posx/MAP_BLOCK)*MAP_BLOCK)-0.0001;//KADA GLEDAMO LIJEVO ILI GORE ODUZIMAMO 0.0001, INACE SE ZRAKA ZAUSTAVI TIK ISPRED ZIDA
			offsetx=-MAP_BLOCK;//GLEDAMO LIJEVO, NEGATIVNO X
			x = posx-rayxinit;//GENERALNO JE MATEMATIKA ISTA KAO KADA TRAZIMO HORIZONTALNU LINIJU NAGORE, SAMO SU X I Y PROMJENILI MJESTA
			initOffsety = x/tang;
			rayyinit = posy+initOffsety;
			offsety = -offsetx/tang;
			for (cntr = 0; cntr<DOF; cntr++)
			{ 
		    	rayy = rayyinit+(cntr*offsety);
				rayx = rayxinit+(cntr*offsetx);
				cellx = (int)(rayx)/MAP_BLOCK;
	        	celly = (int)(rayy)/MAP_BLOCK;
	        	cellpos = celly*MAP_WIDTH+cellx;
	        	if (cellpos<MAP_WIDTH*MAP_HEIGHT && cellpos>=0)
	        	{
	        		cell = map[cellpos];
	        		if(cell!=0)
	        		{
	        			cntr = DOF+1;
	        			distv = sqrt(pow((rayy-posy),2)+pow((rayx-posx),2));
	        			colorV = cell;
	        		}
	       		}
			}
		}
		if (rayAngle>3*PI/2 || rayAngle<PI/2)//GLEDAMO DESNO
		{//NEMA PROMJENA, SVE JE ANALOGNO OSTALIM PRETRAGAMA
			tang = tan(rayAngle);//UOPSTE AKO RAZUMIJES JEDNU OD OVIH PRETRAGA, RAZUMIJES IH SVE
			rayxinit = ((int)(posx/MAP_BLOCK)*MAP_BLOCK)+MAP_BLOCK;
			offsetx = MAP_BLOCK;
			x = rayxinit-posx;
			initOffsety = tang*x;
			rayyinit = posy+initOffsety;
			offsety = tang*offsetx;
			for (cntr = 0; cntr<DOF; cntr++)
			{ 
			    rayy = rayyinit+(cntr*offsety);
				rayx = rayxinit+(cntr*offsetx);
				cellx = (int)(rayx)/MAP_BLOCK;
	        	celly = (int)(rayy)/MAP_BLOCK;
	        	cellpos = celly*MAP_WIDTH+cellx;
	        	if (cellpos<MAP_WIDTH*MAP_HEIGHT && cellpos>=0)
	        	{
	        		cell = map[cellpos];
	        		if(cell!=0)
	        		{
	        			cntr = DOF+1;
	        		 	distv = sqrt(pow((rayy-posy),2)+pow((rayx-posx),2));
	        			colorV = cell;
	        		}
	       		}
			}	
		}
		posyV = rayy;//
		posxV = rayx;//POZICIJE GDJE JE ZRAKA POGODILA ZID U VERTIKALNOJ PRETRAZI
		if (rayAngle==3*PI/2 || rayAngle==PI/2)//VERTIKALNA PRETRAGA NEMA SMISLA AKO GLEDAMO DIREKTNO GORE ILI DIREKTNO DOLE
		{
			distv = 10000;
		}
	//KRAJ PROVJERE DA LI JE ZRAKA POGODILA VERTIKALNI ZID
		if (distv>disth)//KOJI ZID JE ZRAKA POGODILA PRVO? TJ KOJI ZID KAMERA VIDI?(KAMERA NARAVNO VIDI ONAJ ZID KOJI JOJ JE NAJBLIZI)
		{
			rayy = posyH;//SADA JE STVARNA POZICIJA KRAJA ZRAKE ONA POZICIJA U KOJOJ JE POGODILA BLIZI ZID, A UDALJENOST OD TOG ZIDA SMO VEC IZRACUNALI
			rayx = posxH;
			distActual = disth;
			colorActual = colorH;
			hitVert =0;
		}
		else
		{
			rayy = posyV;
			rayx = posxV;
			distActual = distv;
			colorActual = colorV;
			hitVert = 1;
		}
		if(rayCounter == 0)//POZICIJE GDJE SE ZAVRSAVA PRVA I POSLJEDNJA ZRAKA, KASNIJE SE KORISTE ZA OCRTAVANJE VIDNOG POLJA
		{	
		    rayxBegin = rayx;
			rayyBegin = rayy;		
	    }
	    if(rayCounter == FOV-1)
	    {
	    	rayxEnd = rayx;
			rayyEnd = rayy;	
	    }
	    //OBJASNJENJE: STANDARDNO OVA TEHNIKA RENDEROVANJA DOVODI DO EFEKTA RIBLJEG OKA, STO JE LOGICNO JER SU ZIDOVI U CENTRU NASEG VIDNOG POLJA TEHNICKI BLIZE NASEM OKU OD ZIDOVA NA PERIFERIJI
	    //MEDJUTIM U STVARNOM SVIJETU NAS MOZAK ISPRAVI RAZLIKU... IZ TOG RAZLOGA SE ZIDOVI SMANJE PROPORCIONALNO NJIHOVOJ BLIZINI CENTRU
	    rayDelta = rayAngle-posa;//KOLIKO JE ZRAKA BLIZU CENTRU
	    if(rayDelta>2*PI) rayDelta = rayDelta-2*PI;//
	    if (rayDelta<0) rayDelta = 2*PI+rayDelta; //OGRANICENJE NA 0 I 2*PI
		fisheyeFix = cos(rayDelta)*distActual;//NOVA VISINA ZIDA, SMANJENA (ILI POVECANA) ZA COS RAZLIKE ZRAKE I CENTRALNE ZRAKE
		lineHeight = HEIGHT/fisheyeFix *MAP_BLOCK;//VISINA ZIDA, OVDE JE POSTAVLJENO DA JE JEDNAKA SIRINI I DUZINI TJ. 64 JEDINICE
		if(lineHeight>HEIGHT && textu_en == 0) lineHeight=HEIGHT;//OGRANICIMO NA VISINU EKRANA
		heights[rayCounter]=lineHeight;//
		colors[rayCounter]=colorActual;//OVDE CUVAM SVE STO MI TREBA ZA RENDEROVANJE, RENDEROVANJE MORA BITI IZVAN OMP PETLJE
		verts[rayCounter]=hitVert;     //
	}
    }
	for (rayCounter = 0; rayCounter<FOV;rayCounter++)//PETLJA ZA RENDEROVANJE, OVAKO JE NAPRAVLJENO JER SE OPENGL I OPENMP NE MJESAJU
	{
		lineHeight = heights[rayCounter];//
		colorActual = colors[rayCounter];//VRACAMO PROMJENJIVE IZ NIZA
		hitVert = verts[rayCounter];     //
		if(rayCounter == 0)//U OVA DVA IFA SE OCRTAVA VIDNO POLJE
		{
			glColor3f(1,0,1);	
		   	glLineWidth(3);
			glBegin(GL_LINES);
			glVertex2i(posx,posy);
			glVertex2i(rayxBegin,rayyBegin);
			glEnd();
		}
		if(rayCounter == FOV-1)
	    {
	    	glColor3f(1,0,1);	
		   	glLineWidth(3);
			glBegin(GL_LINES);
			glVertex2i(posx,posy);
			glVertex2i(rayxEnd,rayyEnd);
			glEnd();
	    }
		if (textu_en == 0)
		{
			switch(colorActual)//KOJE BOJE JE ZID?
			{
				case 1:
					glColor3f(0,0,1);
					if (hitVert) glColor3f(0,0,0.7);
					break;
				case 2:
					glColor3f(1,0,0);
					if (hitVert) glColor3f(0.7,0,0);
					break;
				case 3:
					glColor3f(0,1,0);
					if (hitVert) glColor3f(0,0.7,0);
					break;	
				case 4:
					glColor3f(1,1,0);
					if (hitVert) glColor3f(0.7,0.7,0);
					break;
				case 5:
					glColor3f(1,1,1);
					if (hitVert) glColor3f(0.7,0.7,0.7);
					break;
			}
			glLineWidth(lineoffset);
			glBegin(GL_LINES);//CRTAMO VERTIKALNI DIO ZIDA ONE BOJE NA ONOJ POZICIJI KOJU DIKTIRA UGAO ZRAKE KAO I UDALJENOST ZIDA OD KAMERE
			glVertex2i(OFFSET+rayCounter*lineoffset,(HEIGHT/2)-(lineHeight/2));
			glVertex2i(OFFSET+rayCounter*lineoffset,(HEIGHT/2)+(lineHeight/2));
			glEnd();
		}
 		
		else//OVAJ DIO JE ZA CRTANJE ZIDA SA TEKSTUROM, EKSPERIMENTALNO I NEBITNO (ALI IZGLEDA FINO), PROMIJENI USE_TEXTURES NA 1 DA VIDIS KAKO IZGLEDA
		{
			glLineWidth(lineoffset);																																		                
			textusize = sizeof textu / sizeof textu[0];                                                       													
			pixelsize = lineHeight/textusize;	                                                           																
			for(i=0;i<textusize;i++)                                                                       
			{                                                                                              
				glColor3f(0.3,1,1); if (hitVert) glColor3f(0.1,0.7,0.7);
				if(textu[i]==0) 	glColor3f(0.5,0.2,0.8);
				if (textu[i]==0 && hitVert) glColor3f(0.3,0.02,0.5);
				glBegin(GL_LINES);
				glVertex2i(OFFSET+rayCounter*lineoffset,(HEIGHT/2)-(lineHeight/2)+(pixelsize*i));
				glVertex2i(OFFSET+rayCounter*lineoffset,(HEIGHT/2)-(lineHeight/2)+pixelsize*i+pixelsize);
				glEnd();	
			}
		}
	}
}
//CRTANJE MAPE
void drawMap()
{
 for (i=0;i<MAP_HEIGHT;i++)//PROLAZIMO KROZ REDOVE MATRICE
 {
 	for (j=0;j<MAP_WIDTH;j++)//KROZ KOLONE MATRICE
 	{
 		switch(map[MAP_WIDTH*i+j])//UKOLIKO JE CELIJA NA PRESJEKU REDA I KOLONE...
		{
			case 0:							//OVAJ SWITCH SAMO BIRA BOJU KRADRATA
				glColor3f(0.2,0.2,0.2);		//ZA 0 STAVLJA SIVU JER JE POD U PITANJU, NE ZID
				break;
			case 1:
				glColor3f(0,0,1);
				break;
			case 2:
				glColor3f(1,0,0);
				break;
			case 3:
				glColor3f(0,1,0);
				break;
			case 4:
				glColor3f(1,1,0);
				break;
			case 5:
				glColor3f(1,1,1);
				break;
		}	                                                  //vvv OFFSET HOR. I VERT. JE U STVARI PROSTOR (HOR. ODNOSNO VERT.)KOJI ZAUZIMAJU DO TADA NACRTANE CELIJE
 		glBegin(GL_QUADS);//CRTAMO SAMU CELIJU, KOJA JE KVADRAT, TJEMENA KVADRATA SE NAVODE U SMIJERU KAZALJKE NA SATU     |X_KOORDINATE/Y_KOORDINATE|
        glVertex2d(j*MAP_BLOCK+MAP_LINE,i*MAP_BLOCK+MAP_LINE);//PRVO TJEME OFFSET HORIZONTALNO/OFFSET VERTIKALNO
        glVertex2d(j*MAP_BLOCK+MAP_LINE,i*MAP_BLOCK+MAP_BLOCK-MAP_LINE);//DRUGO TJEME OFFSET HORIZONTALNO+SIRINA CELIJE/OFFSET VERTIKALNO
        glVertex2d(j*MAP_BLOCK+MAP_BLOCK-MAP_LINE,i*MAP_BLOCK+MAP_BLOCK-MAP_LINE);//TRECE TJEME OFFSET HORIZONTALNO+SIRINA CELIJE/OFFSET VERTIKALNO+VISINA CELIJE
        glVertex2d(j*MAP_BLOCK+MAP_BLOCK-MAP_LINE,i*MAP_BLOCK+MAP_LINE);//CETVRTO TJEME OFFSET HORIZONTALNO/OFFSET VERTIKALNO+VISINA CELIJE
        glEnd();//KRAJ DATOG KVADRATA //NAPOMENA: +-MAP_LINE JE TU CISTO DA OZNACI GRANICE SVIH CELIJA TAKO STO SMANJUJE KVADRATE KOJI PREDSTAVLJAJU CELIJE ZA PO JEDAN PIKSEL
 	} 													 //^^^INACE SE CELIJE SPOJE I TEZE JE ZA VIDJETI STA PROGRAM RADI
 }  
}

//FUNKCIJA KOJA PRIMA UNOS SA TASTATURE, key JE TASTER KOJI JE IZAZVAO INTERRUPT I TIME POZVAO FUNKCIJU
void keyboard(unsigned char key, int x, int y)
{
	if (key == 'a') //ROTACIJA ULIJEVO
	{
	  posa -=0.1;//SMANJI UGAO KAMERE ZA MALU VRIJEDNOST (U RADIJANIMA)
	  if(posa<=0)//AKO ODE ISPOD NULE, VRATITI NA 2*PI (2*PI JE 0)
	  {
	  	posa=2*PI;
	  }
	  distx = cos(posa)*5;//ZA OVE KOLICINE SE KAMERA POMJERI KADA PRITISNEMO W ILI S
	  disty = sin(posa)*5;
	}
	if (key == 'd')//ROTACIJA UDESNO
	{
	  posa +=0.1;//POVECAVANJE UGLA
	  if (posa>=2*PI)//OGRANICENJE NA 2*PI
	  {
	  	posa=0;
	  }
	  distx = cos(posa)*5;
	  disty = sin(posa)*5;//ISTA STVAR, ISTA KALKULACIJA
	}
	if (key == 'w')//KRETANJE UNAPRIJED
	{
	 cellx = (int)(posx+distx)/MAP_BLOCK;//
	 celly = (int)(posy+disty)/MAP_BLOCK;//
	 cellpos = celly*MAP_WIDTH+cellx;	 //
	 cell = map[cellpos];				 //
	 if(cell==0)						 //AKO JE CELIA U KOJU KAMERA TREBA DA UDJE PRITISKOM NA W PRAZNA, TJ AKO JE map[POZICIJA] == 0, POMJERI KAMERU U TU POZICIJU
	 {									 //U SUPROTNOM, NE URADI NISTA, INACE KAMERA ULAZI U ZID
	 posy+=disty;						 //
	 posx+=distx;						 //
     }									 //
	}									 //
	if (key == 's') //KRETANJE UNAZAD
    {
	 cellx = (int)(posx-distx)/MAP_BLOCK;//ISTA KALKULACIJA KOLIZIJE, SAMO STO SADA IDEMO UNAZAD
	 celly = (int)(posy-disty)/MAP_BLOCK;
	 cellpos = celly*MAP_WIDTH+cellx;
	 cell = map[cellpos];
	 if(cell==0)
	 {
	 posy-=disty;
	 posx-=distx;
     }
	}	
	if (key == 'q')//PRITISKOM NA q IZRACUNAVAMO RELEVANTNE VRIJEDNOSTI STO SE TICE PERFORMANSI I PRIKAZUJEMO KORISNIKU...
	{
		avgTime = totalFrameTime/(float)totalFrames;//SREDNJE VRIJEME FREJMOVA, STANDARDNA FORMULA
		framesPerSecond = 1.0000/avgTime * 1000;//*1000 ZBOG MILISEKUNDI
		printf("Average frame time=%f\n",avgTime);
		printf("FPS: %f\n",framesPerSecond);
		printf("Shortest frame time=%f\n",shortestFrame);
		printf("Longest frame time=%f\n",longestFrame);
		printf("Total frames elapsed=%d",totalFrames);
        writeToLog();//...I ISPISUJEMO ISTE U LOG...
		glutDestroyWindow(1);//...I ZATVARAMO PROGRAM
		exit(0);
	} 
	if (key == 'e') 						//UNISTAVANJE BLOKOVA KADA PRITISNEMO e, NIJE BITNO
    {
	 cellx = (int)(posx+5*distx)/MAP_BLOCK;
	 celly = (int)(posy+5*disty)/MAP_BLOCK;
	 cellpos = celly*MAP_WIDTH+cellx;
	 cell = map[cellpos];
	 if(cell!=0)
	 {
	 map[cellpos]=0;
     }
	}
	if (key != 'q')
	glutPostRedisplay();//<-- U SLUCAJU DA NEKI TASTER JESTE PRITISNUT, ODMAH PONOVO POZOVI FUNKCIJU ZA PRIKAZ NA EKRAN
}


//OVA FUNKCIJA JE ANALOGNA FUNKCIJI keyboard(), SAMO STO OMOGUCAVA KONTROLISANJE STRELICAMA
void spec(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_UP:
            cellx = (int)(posx+distx)/MAP_BLOCK;
	 		celly = (int)(posy+disty)/MAP_BLOCK;
	 		cellpos = celly*MAP_WIDTH+cellx;
	 		cell = map[cellpos];
	 		if(cell==0)
	 		{
	 			posy+=disty;
	 			posx+=distx;
     		}
			break;
		case GLUT_KEY_DOWN:
			cellx = (int)(posx-distx)/MAP_BLOCK;
	 		celly = (int)(posy-disty)/MAP_BLOCK;
	 		cellpos = celly*MAP_WIDTH+cellx;
	 		cell = map[cellpos];
	 		if(cell==0)
	 		{
	 			posy-=disty;
	 			posx-=distx;
     		}
			break;
		case GLUT_KEY_LEFT:
	  			posa -=0.1;
	  			if(posa<=0)
	  			{
	  				posa=2*PI;
	  			}
	  			distx = cos(posa)*5;
	  			disty = sin(posa)*5;
				break;
		case GLUT_KEY_RIGHT:
				posa +=0.1;
	  			if (posa>=2*PI)
	  			{
	  				posa=0;
	  			}
	  			distx = cos(posa)*5;
	  			disty = sin(posa)*5;
				break;
}

glutPostRedisplay();

}

//FUNKCIJA KOJA CRTA KAMERU
void drawPlayer()
{
 glColor3f(1,1,0);//BOJA KAMERE (ZUTA)
 glPointSize(8);//VELICINA KAMERE (KAMERA JE PREDSTAVLJENA CETVRTASTOM TACKOM)
 glBegin(GL_POINTS);//CRTAMO CETVRTASTU TACKU(ZA OPIS CRTANJA SA OPENGL, KOMENTARI SU ISPOD main())
 glVertex2i(posx,posy);//POZICIJA KAMERE, TJ POZICIJA TACKE KOJU CRTAMO(A CRTAMO KAMERU)
 glEnd(); 
 rays();//POZIV FUNKCIJE KOJA CRTA ZRAKE----OVDE POSTAJE KOMPLIKOVANIJE
 glColor3f(1,1,0);
 glLineWidth(3);
 glBegin(GL_LINES);
 glVertex2i(posx,posy);
 glVertex2i(posx+(5*distx),posy+(5*disty));
 glEnd();//OVIM SKUPOM FUNKCIJA SMO NACRTALI KRATKU LINIJU OD CENTRA KAMERE U SMIJERU U KOJEM KAMERA GLEDA, DA BI JE BILO LAKSE KONTROLISATI, TJ DA VIDIMO GDJE CE SE KAMERA POKRENUTI KADA PRITISNEMO W ILI S
}

//RADI JEDNOSTAVNOSTI PROZOR ZAKLJUCAVAMO NA DATE DIMENZIJE
void changeSize(int w, int h)
{
	glutReshapeWindow(WIDTH,HEIGHT);
}

//FUNKCIJA KOJA SE BAVI PRIKAZIVANJEM SVEGA
void display()
{
 frameStart = (float)glutGet(GLUT_ELAPSED_TIME);//VRIJEME POCETKA FREJMA
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//NA POCETKU SVAKOG TIKA RESETUJEMO PROZOR
 background();
 drawMap();//FUNKCIJA ZA CRTANJE MAPE
 drawPlayer();//FUNKCIJA ZA CRTANJE KAMERE(I ZRAKA)
 glutSwapBuffers();//PREBACITI INTERNI BUFFER U BUFFER GRAFICKE KARTICE, tj. PRIKAZATI SCENU NA EKRANU
 frameEnd = (float)glutGet(GLUT_ELAPSED_TIME);//VRIJEME KRAJA FREJMA
 frameTime = frameEnd-frameStart;//UKUPNO VRIJEME FREJMA
 totalFrameTime +=frameTime;//UKUPNO VRIJEME SVIH FREJMOVA
 totalFrames++;//UKUPAN BROJ SVIH FREJMOVA
 if(frameTime>longestFrame && totalFrames>SKIP_FRAMES) longestFrame=frameTime;//DA LI JE NAJDUZI FREJM
 if(frameTime<shortestFrame && totalFrames>SKIP_FRAMES && frameTime>0.0000) shortestFrame=frameTime;//DA LI JE NAJKRACI FREJM
 if (bmark_en == 1)
 {
 	Benchmark();
 }
}

//POSTAVLJANJE POCETNIH PARAMETARA...
void init()
{
	glClearColor(0.3,0.3,0.3,0);//BOJA POZADINE PROZORA (SIVA)
	gluOrtho2D(0,WIDTH,HEIGHT,0);//MATRICA PROJEKCIJE ZA DVO-DIMENZIONALNE SCENE, IMA DIMENZIJE PROZORA
	posx=300;
	posy=300;
	posa=3*PI/2;//POZOCIJA I UGAO GLEDANJA KAMERE
	distx = cos(posa)*5;//KADA BI SE KAMERA INICIJALNO POKRENULA NAZAD ILI NAPRIJED, OVO BI BILE PROMJENE POZICIJE NA X, ODNOSNO Y OSI(PROJEKCIJA UGLA KAMERE NA X, ODNOSNO Y OSU)
	disty = sin(posa)*5;//*5 JE ZATO STO JE INACE KRETANJE NEVJEROVATNO SPORO
}

//GLAVNA FUNKCIJA
int main(int argc, char** argv)
{ 
 getParams();//PARAMETRI PROGRAMA
 glutInit(&argc, argv);//ZAPOCINJE FUNKCIONISANJE OPENGL-a, ZAHTIJEVANA OD STRANE OPENGL
 glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);//MOD OPERACIJE- RGBA OZNACAVA NACIN ZA DEFINICIJU BOJA, A DOUBLE NAM DAJE DVA BUFFERA(PISEMO U INTERNI BUFFER, KOJI SE ZATIM PRIKAZE NA EKRANU)
 glutInitWindowSize(WIDTH,HEIGHT);//DEFINICIJA DIMENZIJA PROZORA
 glutCreateWindow("RayCaster");//KREACIJA PROZORA SA DATIM NAZIVOM
 glutReshapeFunc(changeSize);//FUNKCIJA KOJA SE POZIVA KADA SE PROZORU PROMJENE DIMENZIJE OD STRANE KORISNIKA
 init();//FUNKCIJA U KOJOJ POSTAVLJAMO NEKE POCETNE PARAMETRE
 glutDisplayFunc(display);//FUNKCIJA KOJA SE BAVI PISANJEM NA SAM EKRAN
 glutKeyboardFunc(keyboard);//FUNKCIJA KOJA PRIMA UNOS OD KORISNIKA
 glutSpecialFunc(spec);//FUNKCIJA KOJA PRIMA UNOS SPECIJALNIH NE-ASCII KARAKTERA OD KORISNIKA
 glutIdleFunc(display);//STA DA PROGRAM RADI KADA NEMA UNOSA(OVO JE NEOPHODNO DA BI BROJAC FREJMOVA FUNKCIONISAO)
 glutMainLoop();//GLAVNA PETLJA, BESKONACNA JE I ZAHTIJEVANA OD STRANE OPENGL
}

/*OPENGL CRTANJE:
	1.POSTAVIMO BOJU I DEBLJINU LINIJE ILI TACKE SA FUNKCIJAMA COLOR I POINTSIZE, ODNOSNO LINEWIDTH
	2.POZOVEMO FUNKCIJU GLBEGIN I DAMO JOJ KLASU ONOGA STO ZELIMO DA CRTAMO: TACKA, LINIJA, KVADRAT ITD.
	3.NAVODIMO SKUP TACAKA: AKO CRTAMO TACKE, ONE CE SVE BITI NACRTANE, A LINIJE I KVADRATI (UKOLIKO NJIH CRTAMO)SE FORMIRAJU OD NAVEDENIH TACAKA, LINIJE OD PO DVIJE, KVADRATI OD PO CETIRI
	4.POZIVOM FUNKCIJE GLEND PRENOSIMO SVE TO STO SMO NAVELI U BUFFER
	5.POZIVOM FUNKCIJE GLUTSWAPBUFFERS CRTAMO BUFFER NA EKRAN
*/
/*                        # 
   FIGURA 1     |        /                 /<-PREDSTAVLJA ZRAKU
                |       /
 *=prvi hor.    |--~---#---------
    presijek    |  I  /
                |  I /            == JE INITOFFSETX
                |  I/             ,A RACUNA SE SA -Y
 #=ostali       |==*----          JER, OPET, Y IDE ODOZGO NA DOLE
  presijeci     | / <ZRAKA        OCIGLEDNO JE DA JE X POZICIJA GDJE ZRAKA UDARI ZID=POZICIJA KAMERE + OVAJ INITOFFSETX
                |/  
----------------+------------------->X
kamera je u 0,0 |                RAYYINIT SE RACUNA NA DATI NACIN,JER VEC ZNAMO POZICIJU GRANICE DATE CELIJE, A TO JE POZICIJA KAMERE/VISINA CELIJE
,sto je ovdje + V                A ZNAMO DA ZID MOZE BITI SAMO NA GRANICAMA CELIJA, U OVOM SLUCAJU (GLEDAMO NAGORE) SAMO NA DONJOJ GRANICI NEKE CELIJE
                Y
                
SA DATE SLIKE JE VEC OCIGLEDNO DA BI NASLI SLJEDECU Y POZICIJU GDJE ZRAKA PROLAZI KROZ HOR. GRANICU POTREBNO SAMO ODUZETI(Y ODOZGO NA DOLE) VISINU CELIJE OD PRETHODNE POZICIJE, STO JE JEDNOSTAVNA OPERACIJA
DALJE... POVLACIMO LINIJU IZ PRVOG PRESJEKA (*) NAGORE, tj. NORMALNO NA SLJEDECU (GORNJU)HOR. LINIJU (NOVA LINIJA OZNACENA SA SLOVIMA I I IMA DUZINU RAYYINIT), TAKO DOBIJAMO TROUGAO (~,#,*)
OCIGEDNO JE UGAO KOD * JEDNAK UGLU IZMEDJU OSE Y I ZRAKE, TJ UGAO IZMEDJU NASE NOVE LINIJE I OSE X JE ISTI KAO UGAO ZRAKE I OSE X
NA OVAJ NACIN MOZEMO IZRACUNATI SLJEDECU X POZICIJU PRESJEKA ZRAKE I HOR. GRANICA CELIJA, TJ. ONA JE offsety/tang
ZANIMLJIVA STVAR JE U TOME STO JE SVAKA SLJEDECA TACKA U KOJOJ SE ZRAKA SJECE SA HOR. LINIJOM UDALJENA ZA ISTU VRIJEDNOST PO X I Y
OVIM U SVAKOJ ITERACIJI DOBIJAMO PRESJEK SA GRANICOM KAO X+OFFSETX I Y+OFFSETY
*/
/* OBJASNJENJE CLANOVA NIZA ZA BENCHMARK, moves
0- skip
1- move forward
2- turn left
3- turn right
4- move back
5- quit
*/
