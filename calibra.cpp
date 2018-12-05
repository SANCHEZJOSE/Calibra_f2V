#include <time.h>
#include <inttypes.h>
#include <algorithm>    // std::max
#include <unistd.h>
#include <stdio.h>
#include "ADS1256.h"
#include "ini/cpp/INIReader.h"//iniciar variables con archivo de configuraciones
#include <iostream>
#include <string>
#include <sstream>
using namespace std;
//Variables
int clock_divisor=256;///Divisor Reloj (bcm2835 250 Mhz)
float freq_in;	
float vRef = 2.5,tiempo,dt,minutos_Muestro=20,save_min=5,ponderacion=0.0000625;///Volt. de referencia por defecto,Variable t(us)
int filas,columnas=3,sps=150,muestras_anteriores,blink=2;
float **datos;///doble puntero para matriz dinamica
bool estado=false;
//funciones y macros
void setup(void);
float save_data_freq(float **,float,float,float,char *);///Permite guardar los datos en un archivo .txt
float recolecta_Data_f2V(ADS1256 &,float **);///Permite recolectar los datos
///											del conversor ads1256.
struct timespec ts1, ts2;/// estructuras necesarias para calculo de 
///								tiempo en us


int main(int argc,char * argv[])
{	
	setup();
        string mystr;
	bcm2835_init();///Inicia GPIO con la libreria bcm2835
	ADS1256 adc24b(clock_divisor,vRef,true,2,8);///Pre config. de prot. SPI
	printf("ADC 24 Bits begin\n");
    adc24b.begin(ADS1256_DRATE_500SPS,ADS1256_GAIN_1,false);///Configuracion
	printf("Creacion Matriz dinamica\n");
	filas=sps*60.0*save_min;
	    datos = new float* [filas];///vector de punteros
    for (int i = 0; i < filas; i++){
        datos[i] = new float[columnas];///vector de 4 punteros para cada fila
    }
    printf("Iniciando lectura\n");
    
    float min_rec = 0;
    float tbase=0;
    while (min_rec<minutos_Muestro) {
		dt=recolecta_Data_f2V(adc24b,datos);
		cout << "Por favor, ingrese frecuencia (en Hz) ";
//		getline(cin,freq_in);
		cin >> freq_in;
		if (freq_in<0)
		  {
		    printf ("Mediciones terminadas\n");
		    exit (EXIT_FAILURE);
		  }
		tbase=save_data_freq(datos,freq_in,dt,tbase,argv[1]);
//		printf("Datos temporales guardados\n");
		min_rec+=save_min;
    };  
    for (int i = 0; i <filas; i++) {
        delete[] datos[i];
    }
    delete[] datos;
return (0);

}


float recolecta_Data_f2V(ADS1256 & ads24,float ** data)
{
	// programa que recolecta datos de dV para 5 canales
    int i=0;
	muestras_anteriores=0;
	ads24.setChannel(0,1);
	clock_gettime( CLOCK_REALTIME, &ts1 );

	while(i < filas){
		ads24.waitDRDY(); 
		ads24.setChannel(2,3);
		data[i][0]=ads24.readChannel(); // Salida de los pines AIN0 y AIN1
		ads24.waitDRDY();
		ads24.setChannel(4,5);
		data[i][1]=ads24.readChannel(); //// Salida de los pines AIN2 y AIN3     
		ads24.waitDRDY();
		ads24.setChannel(0,1); 
		data[i][2]=ads24.readChannel();  // Salida de los pines AIN4 y AIN5
				if(i-muestras_anteriores>=sps/blink){
		muestras_anteriores=i;
			if(estado==false){
				estado=true;}
				else{
				estado=false;}
				}
		if(estado){ bcm2835_gpio_write(12,HIGH);}
		else{ bcm2835_gpio_write(12,LOW);}
	    i++;
		}
		clock_gettime( CLOCK_REALTIME, &ts2 );
		tiempo=(float) ((1.0*ts2.tv_nsec - ts1.tv_nsec*1.0)*1e-9 +1.0*ts2.tv_sec - 1.0*ts1.tv_sec )*1000.0;
	    return(tiempo/(float)filas);
}
float save_data_freq(float **matriz,float freq_in,float delta,float t_0,char * name){
int aux;
FILE * archivo=fopen(name,"a+");
for (int j = 0; j <columnas; j++){
		fprintf(archivo,"%2d\t",j+1); // colomna, ed, canal
		fprintf(archivo,"%.9f\t",freq_in);     // frecuencia
		for (int i = 0; i < filas; i++){
			fprintf(archivo,"%.9f\t",matriz[i][j]);
		aux=i;
		}

		fprintf(archivo,"\n");
        }


	fclose(archivo);
	return delta*aux+t_0;
	}
void setup(){
	    INIReader reader("config.ini");

    if (reader.ParseError() < 0) {
        printf( "Error! el archivo de config no se encuentra.\n Variables iniciadas con valores por defecto\n");

    }
    else{ minutos_Muestro = (float)reader.GetReal("configuraciones", "minutos",20.0);
           printf("TIEMPO DE MUESTREO : %f MINUTOS\n", minutos_Muestro );
           save_min = (float)reader.GetReal("configuraciones", "save",0.5);
           printf("TIEMPO PARA GUARDAR : %f MINUTOS\n", save_min );
           sps = reader.GetInteger("configuraciones", "sps",150);
           printf("MUESTRAS POR SEGUNDO %d SPS\n", sps );
           clock_divisor = reader.GetInteger("configuraciones","clock_divisor",256);
           printf("DIVISOR DEL RELOJ(BCM2835) : %d \n", clock_divisor );
           blink = reader.GetInteger("configuraciones","blink",1);
           printf("DIVISOR DE PARPADEO LED : %d \n", blink );
           printf("Variables configuradas segun 'configuraciones.ini'\n");
           }
	}
