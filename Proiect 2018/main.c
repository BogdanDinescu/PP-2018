#include <stdio.h>
#include <stdlib.h>

typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef struct{
    UCHAR b,g,r;
}pixel;

typedef union{
    UINT x;
    UCHAR octet[4];
}Bytes;

void afisarev(UINT v[],UINT n){
    int i;
    for(i=0;i<n;i++)
        printf("%u ",v[i]);
    printf("\n");
}


pixel xorpixel(pixel p1,pixel p2,UINT r){
    Bytes b;
    b.x=r;
    p1.b=p1.b^p2.b^b.octet[0];
    p1.g=p1.g^p2.g^b.octet[1];
    p1.r=p1.r^p2.r^b.octet[2];
    return p1;
}

void permutare_inversa(UINT **v,UINT **u,int n){
  *u = malloc(sizeof(UINT)*n);
  int i;
  for (i=0;i<n;i++)
    (*u)[(*v)[i]] = i;
  free(*v);
}

void xorshift32(UINT **v,UINT n,UINT x){
    int i;
    *v=malloc(n*sizeof(UINT));
    for(i=0;i<n;i++){
        x = x ^ x<<13;
        x = x ^ x>>17;
        x = x ^ x<<15;
        (*v)[i] = x;
    }
}

void durstenfeld(UINT **v,UINT *r,UINT n){
    UINT i,aux,rand;
    *v = malloc(sizeof(UINT)*n);
    for(i=0;i<n;i++)
        (*v)[i]=i;
    for(i=n-1;i>=1;i--){
        rand=r[i]%(i+1);
        aux=(*v)[rand];
        (*v)[rand]=(*v)[i];
        (*v)[i]=aux;
    }
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

void loadimage(char *nume_fisier,pixel **I,UCHAR **header){
    FILE *f = fopen(nume_fisier,"rb");
    if(f==NULL){
        printf("eroare la deschiderea imaginii");
        return;
    }
    UINT w,h;
    *header = malloc(54*sizeof(UCHAR));
    fread(*header,sizeof(UCHAR),54,f);
    get_width_height(*header,&w,&h);
    *I = malloc(w*h*sizeof(pixel));

    int i,j,padding,k=0;
    if(w%4!=0)
        padding = 4-(3*w)%4;
    else
        padding = 0;
    for(i=0;i<h;i++){
        for(j=0;j<w;j++)
            fread(&(*I)[k++], 3, 1, f);
        fseek(f,padding,SEEK_CUR);
        }
	fclose(f);
}

void saveimage(char *nume_fisier,pixel **I,UCHAR **header){
       FILE *f = fopen(nume_fisier,"wb");
       fwrite(*header,sizeof(UCHAR),54,f);
       UINT w,h;
       get_width_height(*header,&w,&h);
        int i,j,padding,k=0;
        if(w%4!=0)
            padding = 4-(3*w)%4;
        else
            padding = 0;
        for(i=0;i<h;i++){
            for(j=0;j<w;j++)
                fwrite(&(*I)[k++], 3, 1, f);
            fwrite(&w, padding, 1, f);
        }
        free(*header);
        free(*I);
        fclose(f);
}

void criptare(char *nume_imagine_initiala,char *nume_imagine_criptata,char *cheia_secreta){
    pixel *I,*I2;
    UCHAR *header;
    UINT r0,sv,w,h,*r,*d;
    int i,k;
    //deschidem fisierul cu cheia secreta si citim cele doua numere
    FILE *cheie = fopen(cheia_secreta,"r");
    fscanf(cheie,"%u",&r0);
    fscanf(cheie,"%u",&sv);
    fclose(cheie);
    //incarcam imaginea intr-o matrice liniarizata I
    loadimage(nume_imagine_initiala,&I,&header);
    get_width_height(header,&w,&h);
    I2=malloc(w*h*sizeof(pixel));
    //gen numerele aleatoare cu xorshift in r
    xorshift32(&r,2*w*h,r0);
    //generam permutarea in d si permutam
    durstenfeld(&d,r,w*h);
    for(i=0;i<w*h;i++)
        I2[d[i]]=I[i];
    free(I);
    //aplicam cripatrea
    Bytes b,c;
    b.x=sv;
    c.x=r[w*h];
    //pentru k=0
    I2[0].b=b.octet[0]^I2[0].b^c.octet[0];
    I2[0].g=b.octet[1]^I2[0].g^c.octet[1];
    I2[0].r=b.octet[2]^I2[0].r^c.octet[2];
    //pentru k>0
    for(k=1;k<w*h;k++)
        I2[k]=xorpixel(I2[k-1],I2[k],r[w*h+k]);
    free(r);
    free(d);
    saveimage(nume_imagine_criptata,&I2,&header);
}

void decriptare(char *nume_imagine_initiala,char *nume_imagine_decriptata,char *cheia_secreta){
    pixel *I,*I2;
    UCHAR *header;
    UINT r0,sv,w,h,*r,*d,*d2;
    int i,k;
    //deschidem fisierul cu cheia secreta si citim cele doua numere
    FILE *cheie = fopen(cheia_secreta,"r");
    fscanf(cheie,"%u",&r0);
    fscanf(cheie,"%u",&sv);
    fclose(cheie);
    //incarcam imaginea intr-o matrice liniarizata I
    loadimage(nume_imagine_initiala,&I,&header);
    get_width_height(header,&w,&h);
    I2=malloc(w*h*sizeof(pixel));
    //gen numerele aleatoare cu xorshift in r
    xorshift32(&r,2*w*h,r0);
    //generam permutarea in d
    durstenfeld(&d,r,w*h);
    //facem inversa lui d in d2
    permutare_inversa(&d,&d2,w*h);
    //aplicam decripatrea
    Bytes b,c;
    b.x=sv;
    c.x=r[w*h];
    //pentru k>0
    for(k=w*h-1;k>=1;k--)
        I[k]=xorpixel(I[k],I[k-1],r[w*h+k]);
    //pentru k=0
    I[0].b=b.octet[0]^I[0].b^c.octet[0];
    I[0].g=b.octet[1]^I[0].g^c.octet[1];
    I[0].r=b.octet[2]^I[0].r^c.octet[2];
    //permutam dupa inversa lui d
    for(i=0;i<w*h;i++)
        I2[d2[i]]=I[i];
    free(I);
    free(r);
    free(d2);
    saveimage(nume_imagine_decriptata,&I2,&header);
}

void chipatrat(char *nume_imagine){
    FILE *f = fopen(nume_imagine,"rb");
    int w,h,i,j,padding;
    float chipatratR=0,chipatratG=0,chipatratB=0,fbara;
    UINT *B,*G,*R;
    pixel p;
    B=calloc(256,sizeof(UINT));
    G=calloc(256,sizeof(UINT));
    R=calloc(256,sizeof(UINT));
    fseek(f,18,SEEK_SET);
    fread(&w,sizeof(int),1,f);
    fread(&h,sizeof(int),1,f);
    fbara=(w*h)/256;
    if(w%4!=0)
        padding = 4-(3*w)%4;
    else
        padding = 0;
    fseek(f,54,SEEK_SET);
    for(i=0;i<h;i++){
        for(j=0;j<w;j++){
            fread(&p,sizeof(pixel),1,f);
            B[p.b]++;
            G[p.g]++;
            R[p.r]++;
        }
        fseek(f,padding,SEEK_CUR);
    }
    for(i=0;i<=255;i++){
        chipatratB+=((B[i]-fbara)*(B[i]-fbara))/fbara;
        chipatratG+=((G[i]-fbara)*(G[i]-fbara))/fbara;
        chipatratR+=((R[i]-fbara)*(R[i]-fbara))/fbara;
    }
    free(B);
    free(G);
    free(R);
    printf("Testul chi patrat:\nPe canalul Red: %f, pe canalul Green: %f, pe canalul Blue: %f\n",chipatratR,chipatratG,chipatratB);
    fclose(f);
}

int main()
{
    criptare("peppers.bmp","criptat.bmp","secret_key.txt");
    decriptare("criptat.bmp","decriptat.bmp","secret_key.txt");
    chipatrat("peppers.bmp");
}
