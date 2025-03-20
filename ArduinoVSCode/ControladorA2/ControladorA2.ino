/*
Arduino 2, Trabajo de Titulación.
Fecha de inicio: 28 de febrero de 2025.
*/

// Librerías.
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <TaskScheduler.h>
#include <String.h> 

// Interfaz para inicializar los componentes.
class IInicializador {
public:
    virtual void init() = 0;
    virtual ~IInicializador() {}
};

// Interfaz para el Observador en el patrón Observator.
class Observador {
public:
    virtual void update(const String& sensor, const String& valor) = 0;
    virtual ~Observador() {}
};

// Interfaz para el Sujeto en el patrón Observator.
class Sujeto {
private:
    Observador* observadores[10]; // Array de punteros a Observador
    int numObservadores = 0; // Contador de observadores

public:
    //Método para ejecutar la notificación de todos lso componentes.
    virtual void componenteNotificar() = 0;
    // Método para agregar un observador.
    void agregarObservador(Observador* observador) {
        if (numObservadores < 10) {
            observadores[numObservadores++] = observador;
        }
        else {
            Serial.println("Limite de observadores alcanzado.");
        }
    }

    // Método para quitar el observador.
    void quitarObservador(Observador* observador) {
        for (int i = 0; i < numObservadores; i++) {
            if (observadores[i] == observador) {
                for (int j = i; j < numObservadores - 1; j++) {
                    observadores[j] = observadores[j + 1];
                }
                numObservadores--;
                break;
            }
        }
    }

protected:
    // Método para notificar el valor de los sensores.
    void notificar(const String& sensor, const String& valor) {
        for (int i = 0; i < numObservadores; i++) {
            observadores[i]->update(sensor, valor);
        }
    }
};

// Interfaz para el Actuador.
class IActuador {
public:
    virtual void ejecutarAccion(int valor) = 0;
    virtual ~IActuador() {}
};

class IComunicacion {
    public:
        virtual void iniciar() = 0;
        virtual String recibirMensaje() = 0; // Ahora devuelve el mensaje
        virtual void enviarMensaje(const String& mensaje, const String& topico) = 0;
        virtual void actualizar() = 0; // Método común para tareas de mantenimiento
        virtual ~IComunicacion() {}
};

class ComunicacionSerial: public IComunicacion{
    private:
        SoftwareSerial& serialCom; //Instancia para la comunicación serial.
        char mensajeRecibido[256]; //Variable para guardar el mensaje.
        int indiceRecibido; //Controla la ubicación del buffer.

    public:
        //Método constructor.
        ComunicacionSerial(SoftwareSerial& serial) : serialCom(serial), indiceRecibido(0){
            mensajeRecibido[0] = '\0'; //Inicializar el buffer.
        }

        //Método para iniciar la comunicación.
        void iniciar() override{
            serialCom.begin(115200);
            Serial.begin(115200);
        }

        //Función para recibir el mensaje.
        String recibirMensaje() override{
            while (serialCom.available() > 0)
            {
                char caracter = serialCom.read(); 

                if (caracter == '$'){
                    mensajeRecibido[indiceRecibido] = '\0';
                    if (mensajeRecibido[0] == '#' && mensajeRecibido[1] == ';'){
                        String mensajeCompleto = String(mensajeRecibido);
                        indiceRecibido = 0;
                        mensajeRecibido[0] = '\0';
                        return mensajeCompleto;
                    }
                    indiceRecibido = 0;
                    mensajeRecibido[0] = '\0';
                }
                else if (indiceRecibido < sizeof(mensajeRecibido)-1){
                    if (indiceRecibido == 0 && caracter != '#'){
                        continue;
                    }
                    mensajeRecibido[indiceRecibido++] = caracter;
                } else {
                    Serial.print("Error: Buffer Lleno.");
                    indiceRecibido = 0;
                    mensajeRecibido[0] = '\0';
                }
            
            }
            return = ""; //Devuelve vacio si no está el mensaje completo.
            
        }
            
        

        //Método para enviar el mensaje.
        void enviarMensaje(const)
    

        

}; //Fin de la clase ComunicacionSerial,