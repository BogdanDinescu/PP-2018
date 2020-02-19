#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef struct{
    UCHAR b,g,r;
}pixel;

typedef struct{
    UCHAR r,g,b;
}culoare;

typedef struct{
    int x,y;
}punct;

typedef struct{
    punct p;
    float corr;
    int cifra;
}detectie;

void grayscale(char* nume_fisier_sursa,char* nume_fisier_destinatie)
{
   FILE *fin, *fout;
   UINT dim_img, latime_img, inaltime_img;
   UCHAR *pRGB, aux;
   pRGB=malloc(sizeof(UCHAR)*3);
   fin = fopen(nume_fisier_sursa, "rb");
   if(fin == NULL)
   	{
   		printf("nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fout = fopen(nume_fisier_destinatie, "wb+");

   fseek(fin, 2, SEEK_SET);
   fread(&dim_img, sizeof(UINT), 1, fin);

   fseek(fin, 18, SEEK_SET);
   fread(&latime_img, sizeof(UINT), 1, fin);
   fread(&inaltime_img, sizeof(UINT), 1, fin);

	fseek(fin,0,SEEK_SET);
	unsigned char c;
	while(fread(&c,1,1,fin)==1)
	{
		fwrite(&c,1,1,fout);
		fflush(fout);
	}
	fclose(fin);

	int padding;
    if(latime_img % 4 != 0)
        padding = 4 - (3 * latime_img) % 4;
    else
        padding = 0;

	fseek(fout, 54, SEEK_SET);
	int i,j;
	for(i = 0; i < inaltime_img; i++)
	{
		for(j = 0; j < latime_img; j++)
		{
			fread(pRGB, 3, 1, fout);
			aux = 0.299*pRGB[2] + 0.587*pRGB[1] + 0.114*pRGB[0];
			pRGB[0] = pRGB[1] = pRGB[2] = aux;
        	fseek(fout, -3, SEEK_CUR);
        	fwrite(pRGB, 3, 1, fout);
        	fflush(fout);
		}
		fseek(fout,padding,SEEK_CUR);
	}
	free(pRGB);
	fclose(fout);
}

void get_width_height(UCHAR *header,UINT *w,UINT *h){
    *w=*h=0;
    int p = 1,i;
    UCHAR x;
    for(i=0;i<=3;i++){
        x=header[18+i];
        *w += x*p;
        p *= 256;
    }
    p = 1;
    for(i=4;i<=8;i++){
        x=header[18+i];
        *h += x*p;
        p *= 256;
    }
}

void intoarce_imagine(pixel ***I,UCHAR *header){
    UINT w,h;
    get_width_height(header,&w,&h);
    pixel *aux;
    int i;
    for(i=0;i<h/2;i++){
        aux=(*I)[i];
        (*I)[i]=(*I)[h-1-i];
        (*I)[h-1-i]=aux;
        }
}

void loadimage(char *nume_fisier,pixel ***I,UCHAR **header){
    FILE *f = fopen(nume_fisier,"rb");
    if(f==NULL){
        printf("eroare la deschiderea imaginii");
        return;
    }
    UINT w,h;
    *header = malloc(54*sizeof(UCHAR));
    fread(*header,sizeof(UCHAR),54,f);
    get_width_height(*header,&w,&h);
    *I = malloc(sizeof(pixel *)*h);
    int i,j;
       for(i=0;i<h;i++){
            (*I)[i]=malloc(sizeof(pixel)*w);
            for(j=0;j<w;j++)
                fread(&(*I)[i][j], 3, 1, f);
            }
    intoarce_imagine(I,*header);
	fclose(f);
}

void saveimage(char *nume_fisier,pixel ***I,UCHAR **header){
       FILE *f = fopen(nume_fisier,"wb");
       int i,j;
       fwrite(*header,sizeof(UCHAR),54,f);
       UINT w,h;
       get_width_height(*header,&w,&h);
       intoarce_imagine(I,*header);
       for(i=0;i<h;i++)
            for(j=0;j<w;j++)
                fwrite(&(*I)[i][j], 3, 1, f);
        free(*header);
        for(i=0;i<h;i++)
            free((*I)[i]);
        free(*I);
        fclose(f);
}

int cmp(const void *p, const void *q){
    float x = ((detectie *)p)->corr;
    float y = ((detectie *)q)->corr;
    if(x>y)
        return -1;
    if(x<y)
        return 1;
    return 0;
}


float medie_intensitati(pixel **I,int w,int h,punct colt1,punct colt2){
    int i,j;
    float s=0;
    for(i=colt1.x;i<=colt2.x;i++)
        for(j=colt1.y;j<=colt2.y;j++)
            s+=I[i][j].r;
    return s/(w*h);
}

float deviatia_standard(int n,pixel **I,int w,int h,punct colt1,punct colt2){
    float media = medie_intensitati(I,w,h,colt1,colt2),s=0;
    int i,j;
    for(i=colt1.x;i<=colt2.x;i++)
        for(j=colt1.y;j<=colt2.y;j++)
            s+=(I[i][j].r-media)*(I[i][j].r-media);
    s=s/(n-1);
    s=sqrt(s);
    return s;
}

int obtine_colturile(punct colt1,punct *colt2,UINT w,UINT h){
    if(colt1.x+14>h)
        return 0;
    else
        (*colt2).x=colt1.x+14;

    if(colt1.y+10>w)
        return 0;
    else
        (*colt2).y=colt1.y+10;
    return 1;
}

float corr(int n,pixel **I,UINT w,UINT h,pixel **S,punct colt1){
    punct colt2,colts1,colts2;
    colts1.x=0;
    colts1.y=0;
    colts2.x=14;
    colts2.y=10;
    if(obtine_colturile(colt1,&colt2,w-1,h-1)==0)
        return 0;
    float mediaI = medie_intensitati(I,11,15,colt1,colt2);
    float mediaS = medie_intensitati(S,11,15,colts1,colts2);
    int i,j;
    float s=0,fractie;
    fractie=deviatia_standard(n,I,11,15,colt1,colt2)*deviatia_standard(n,S,11,15,colts1,colts2);
    fractie=1/fractie;
    for(i=colt1.x;i<=colt2.x;i++)
        for(j=colt1.y;j<=colt2.y;j++)
            s+=(I[i][j].r-mediaI)*(S[i-colt1.x][j-colt1.y].r-mediaS);
    s=fractie*s;
    s=s/n;
    return s;
}

void template_matching(char *nume_imagine,char *nume_sablon,float prag,detectie **D,int *n,int cifra){
    pixel **I,**S;
    UCHAR *header,*header2;
    UINT w,h;
    float c;
    punct colt;
    detectie *aux;
    colt.x=0;
    colt.y=0;
    int i,j;
    loadimage(nume_imagine,&I,&header);
    loadimage(nume_sablon,&S,&header2);
    get_width_height(header,&w,&h);
    for(i=0;i<h;i++)
        for(j=0;j<w;j++){
            colt.x=i;
            colt.y=j;
            c=corr(11*15,I,w,h,S,colt);
            if(c>prag){
                (*n)++;
                aux=realloc(*D,sizeof(detectie)*(*n));
                if(aux==NULL){
                    printf("memorie insuficienta");
                    *n=0;
                    return;
                }
                *D=aux;
                (*D)[*n-1].p.x=i;
                (*D)[*n-1].p.y=j;
                (*D)[*n-1].corr=c;
                (*D)[*n-1].cifra=cifra;
            }
        }
    free(*I);
    free(*S);
}

int arie(punct colt1,punct colt2){
    int w,h;
    h=colt2.x-colt1.x+1;
    w=colt2.y-colt1.y+1;
    return w*h;
}

float suprapunere(punct colt1,punct colt3){
    punct colt2,colt4;
    colt2.x=colt1.x+14;
    colt2.y=colt1.y+10;
    colt4.x=colt3.x+14;
    colt4.y=colt3.y+10;
    int ac=arie(colt3,colt2);
    float s=(float)ac/(arie(colt1,colt2)+arie(colt3,colt4)-ac);
    return s;
}

void sorteaza(detectie **D,int nr){
    qsort(*D,nr,sizeof(detectie),cmp);
}

void elim_non_maxime(detectie **D,int *nr){
    sorteaza(D,*nr);
    int i,j,k;
    for(i=0;i<*nr;i++)
        for(j=i+1;j<*nr;j++)
            if(suprapunere((*D)[i].p,(*D)[j].p)>0.2){
                for(k=j;k<*nr-1;k++)
                    (*D)[k]=(*D)[k+1];
                j--;
                (*nr)--;
                }
}

void deseneaza_contur(pixel ***I,punct colt1,punct colt2,culoare c){
    int i;
    for(i=colt1.y;i<=colt2.y;i++){
        (*I)[colt1.x][i].r = c.r;
        (*I)[colt1.x][i].g = c.g;
        (*I)[colt1.x][i].b = c.b;
        (*I)[colt2.x][i].r = c.r;
        (*I)[colt2.x][i].g = c.g;
        (*I)[colt2.x][i].b = c.b;
    }
    for(i=colt1.x;i<=colt2.x;i++){
        (*I)[i][colt1.y].r = c.r;
        (*I)[i][colt1.y].g = c.g;
        (*I)[i][colt1.y].b = c.b;
        (*I)[i][colt2.y].r = c.r;
        (*I)[i][colt2.y].g = c.g;
        (*I)[i][colt2.y].b = c.b;
    }
}

int main()
{
   //definire culori
    culoare c0,c1,c2,c3,c4,c5,c6,c7,c8,c9;
    //rosu
    c0.r=255;
    c0.g=0;
    c0.b=0;
    //galben
    c1.r=255;
    c1.g=255;
    c1.b=0;
    //verde
    c2.r=0;
    c2.g=255;
    c2.b=0;
    //cyan
    c3.r=0;
    c3.g=255;
    c3.b=255;
    //magenta
    c4.r=255;
    c4.g=0;
    c4.b=255;
    //albastru
    c5.r=0;
    c5.g=0;
    c5.b=255;
    //argintiu
    c6.r=192;
    c6.g=192;
    c6.b=192;
    //portocaliu
    c7.r=255;
    c7.g=140;
    c7.b=0;
    //magenta
    c8.r=128;
    c8.g=0;
    c8.b=128;
    //albastru
    c9.r=0;
    c9.g=0;
    c9.b=128;
    grayscale("test.bmp","testgrey.bmp");
    grayscale("cifra0.bmp","cifra0g.bmp");
    grayscale("cifra1.bmp","cifra1g.bmp");
    grayscale("cifra2.bmp","cifra2g.bmp");
    grayscale("cifra3.bmp","cifra3g.bmp");
    grayscale("cifra4.bmp","cifra4g.bmp");
    grayscale("cifra5.bmp","cifra5g.bmp");
    grayscale("cifra6.bmp","cifra6g.bmp");
    grayscale("cifra7.bmp","cifra7g.bmp");
    grayscale("cifra8.bmp","cifra8g.bmp");
    grayscale("cifra9.bmp","cifra9g.bmp");
    detectie *D;
    D=malloc(sizeof(detectie));
    pixel **I;
    UCHAR *header;
    punct colt2;
    loadimage("test.bmp",&I,&header);
    int i,nr=0;
    template_matching("testgrey.bmp","cifra0g.bmp",0.5,&D,&nr,0);printf("0");
    template_matching("testgrey.bmp","cifra1g.bmp",0.5,&D,&nr,1);printf("1");
    template_matching("testgrey.bmp","cifra2g.bmp",0.5,&D,&nr,2);printf("2");
    template_matching("testgrey.bmp","cifra3g.bmp",0.5,&D,&nr,3);printf("3");
    template_matching("testgrey.bmp","cifra4g.bmp",0.5,&D,&nr,4);printf("4");
    template_matching("testgrey.bmp","cifra5g.bmp",0.5,&D,&nr,5);printf("5");
    template_matching("testgrey.bmp","cifra6g.bmp",0.5,&D,&nr,6);printf("6");
    template_matching("testgrey.bmp","cifra7g.bmp",0.5,&D,&nr,7);printf("7");
    template_matching("testgrey.bmp","cifra8g.bmp",0.5,&D,&nr,8);printf("8");
    template_matching("testgrey.bmp","cifra9g.bmp",0.5,&D,&nr,9);printf("9");
    sorteaza(&D,nr);
    elim_non_maxime(&D,&nr);
    for(i=0;i<nr;i++){
        obtine_colturile(D[i].p,&colt2,499,399);
        switch(D[i].cifra){
        case 0:
            deseneaza_contur(&I,D[i].p,colt2,c0);
            break;
        case 1:
            deseneaza_contur(&I,D[i].p,colt2,c1);
            break;
        case 2:
            deseneaza_contur(&I,D[i].p,colt2,c2);
            break;
        case 3:
            deseneaza_contur(&I,D[i].p,colt2,c3);
            break;
        case 4:
            deseneaza_contur(&I,D[i].p,colt2,c4);
            break;
        case 5:
            deseneaza_contur(&I,D[i].p,colt2,c5);
            break;
        case 6:
            deseneaza_contur(&I,D[i].p,colt2,c6);
            break;
        case 7:
            deseneaza_contur(&I,D[i].p,colt2,c7);
            break;
        case 8:
            deseneaza_contur(&I,D[i].p,colt2,c8);
            break;
        case 9:
            deseneaza_contur(&I,D[i].p,colt2,c9);
            break;
        }
    }
    saveimage("rezultat.bmp",&I,&header);
}
