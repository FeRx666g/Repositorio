#include <Arduino.h>
#line 1 "C:\\Users\\ferx666g\\Desktop\\Software\\Trabajo de Titulación\\ProyectoTitulacion\\ArduinoVSCode\\ControladorA1\\ControladorA1.ino"
/* Trabajo de Titulación */
/* Arduino Recolector de Datos*/

// --- Librerias a utilizar ---//
#include <Wire.h> // Para I2C
#include <BH1750.h> // Para el sensor de luz BH1750
#include <Servo.h>  // Para los servomotores
#include <dht11.h> // Para el sensor DHT11
#include <TaskScheduler.h> // Para la programación de tareas
#include <String.h> // Para strings
#include <math.h> // Para operaciones matemáticas
#include <SoftwareSerial.h> // Para la comunicación

class Mensaje {
    String mensaje; //Almacena el mensaje;

    public:
        Mensaje(const String& msg) : mensaje(msg){}

        String getMensaje() const
        {
            return mensaje;
        }
}; //Fin de la clase Mensaje.

//Clase que construye el mensaje.
class MensajeBuilder{
    private:
        char mensaje[256];
        int indice;
    public:
        MensajeBuilder() : indice(0){
            mensaje[0] = '\0';
        }
        //Junta los valores de todos los sensores.
        void agregar(const String& componente, const String& valor) {
            if (indice > 0) {
                                
                indice += snprintf(mensaje + indice, sizeof(mensaje) - indice, ";");
                if (indice >= sizeof(mensaje)) { //Siempre revisar.
                    goto truncar;
                }
            }

            indice += snprintf(mensaje + indice, sizeof(mensaje) - indice, "%s%s", componente.c_str(), valor.c_str());
            
            if (indice >= sizeof(mensaje)) {
                truncar:
                Serial.println("¡Error!  Mensaje demasiado largo.  Truncando...");
                mensaje[sizeof(mensaje) - 1] = '\0';
                indice = sizeof(mensaje) - 1;
            }

        }
        // Método para construir el mensaje final y devolver un objeto Mensaje.
        Mensaje construir() {
                // Construimos el String paso a paso.
            String resultado = "#;";      // Iniciamos con "#;"
            resultado += String(mensaje); // Agregamos el contenido de 'mensaje'
            resultado += ";$%";          // Agregamos "$%"
            return Mensaje(resultado);
        }

        // NUEVO MÉTODO: Limpiar explícitamente el buffer.
        void limpiar() {
            indice = 0;
            mensaje[0] = '\0';
        }

}; //Fin de la clase MensajeBuilder.


//-----------------------//
//----- Interfaces ------//
//-----------------------//

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
        } else {
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

//Interfaz para el movimiento.
class IMovimiento{
    public:
        virtual ~IMovimiento() {}
};

//Interfaz para el patrón Strategy.
class ICalculadorAngulo{
    public:
        virtual int calcularAngulo(int L, int R, int D, int U) = 0;
        virtual ~ICalculadorAngulo (){}
};

//Interfaz para el controlador del servo motor.
class IControladorServo {
    public:
        virtual void actualizarMovimiento(int anguloDeseado) = 0;
        virtual ~IControladorServo() {}
};

/*--------------------------*/
/*----- Controladores ------*/
/*--------------------------*/

//---------------------------------------//
//----- Clase para el sensor de luz -----//
//---------------------------------------//
class BH1750Component : public Sujeto, public IInicializador {
private:
    BH1750 sensor; // Variable para el sensor.
public:
    // Método para inicializar el sensor.
    void init() {
        sensor.begin(); // Inicialización del sensor.
    }

    //Función get para devolver el valor de lux en entero.
    int getLux(){
        int valor = 0;; // Variable para guardar el valor en String.
        // Controlador de los valores del sensor.
        if (sensor.readLightLevel() < 0) {
            valor = 9999; // Si lux es menor a cero se asigna 9999.
        } else {
            valor = sensor.readLightLevel(); // Asignar a la variable el valor de lux en string.
        }
      return valor;
    }

    // Método para leer el valor del sensor y notificar a los observadores.
    void componenteNotificar() override{
        String valor = String(getLux());
        notificar("LuX:", valor); // Notificación a los observadores.
    }
}; // Fin de la clase BH1750Component.

//-----------------------------------------------------//
//----- Clase para el sensor de temperatura DHT11 -----//
//-----------------------------------------------------//
class TemperaturaComponent : public Sujeto, public IInicializador {
private:
    dht11 sensor; // Variable para el sensor.
    const int pin; // Pin del sensor.
public:
    //Método constructor.
    TemperaturaComponent(int pinSensor) : pin(pinSensor) {}

    // Método para inicializar el sensor.
    void init() override {
        // No se necesita inicialización adicional para el DHT11
    }

    //Función get para devolver el valor de la temperatura en entero.
    int getTemperatura() {
        int chk = sensor.read(pin);
        int valor = 0;
        if (chk == DHTLIB_OK) {
            valor = sensor.temperature;
        } else {
            valor = 9999;
        }
        return valor;
    }

    //Método para el valor de la temperatura.
    void componenteNotificar() override{
        String valor = String(getTemperatura());
        notificar("Temperatura:", valor); // Notifica a los observadores
    }
}; //Fin de la clase TemperaturaComponent.

//----------------------------------------------------//
//----- Clase para el sensor de humedad DHT11 -----//
//----------------------------------------------------//
class HumedadComponent : public Sujeto, public IInicializador {
private:
    dht11 sensor; // Variable para el sensor.
    const int pin; // Pin del sensor.
public:
    //Método constructor.
    HumedadComponent(int pinSensor) : pin(pinSensor) {}

    // Método para inicializar el sensor.
    void init() override {
        // No se necesita inicialización adicional para el DHT11
    }

    //Función get para devolver el valor de la humedad en entero.
    int getHumedad() {
        delay(25);
        int chk = sensor.read(pin);
        int valor = 0;
        if (chk == DHTLIB_OK) {
            valor = sensor.humidity;
        } else {
            valor = 9999;
        }
        return valor;
    }

    //Método para el valor de la humedad.
    void componenteNotificar() override{
        String valor = String(getHumedad());
        notificar("Humedad:", valor); // Notifica a los observadores
    }

    
}; //Fin de la clase HumedadComponent.

//------------------------------------------------//
//----- Clase para el sensor de fotoresistor -----//
//------------------------------------------------//
class FotoresistorComponent : public Sujeto, public IInicializador {
  private:
    const int pin; // Pin del sensor
  public:
    //Método constructor.
    FotoresistorComponent(int pinSensor): pin(pinSensor){}

    //Inicializador de los fotoresistores.
    void init() override{
    }
    //Función para obtener el valor del Fotoresistor.
    int getFotoresistor(){
        return analogRead(pin);
    }
    //Método para notificar.
    void componenteNotificar() override{
      notificar("FotoRes" + String(pin) + ":", String(getFotoresistor()));
    }
}; //Fin de la clase FotoresistorComponent.

//-------------------------------------------//
//------- Clase para los servomotores -------//
//-------------------------------------------//
class ServoComponent : public Sujeto, public IInicializador{
  
private:
    Servo servo; // Variable para el servo.
    const int pin; // Pin del servo.
public:
    //Método constructor.
    ServoComponent(int pinServo) : pin(pinServo) {}

    //Inicializador de los servomotores.
    void init() override{
      servo.attach(pin);
      servo.write(0);   
    }

    //Método para mover el servo.
    void moverServo(int angulo){
      servo.write(angulo);
    }

    //Función get para devolver el valor del angulo del servo.
    int getAngulo(){
      return servo.read();
    }

    //Método para leer y notificar.
    void componenteNotificar() override{
      notificar("Servo" + String(pin) + ":", String(getAngulo()));
    }
}; //Fin de la clase ServoComponent.
    
//Clase para calcular el ángulo con fuerzas vectoriales.
class CalcularAnguloFuerzas: public ICalculadorAngulo{
    public:
    int calcularAngulo(int L, int R, int D, int U) override{
        int X_resultante = L - R; //Fuerza en X.
        int Y_resultante = U - D; //Fuerza en Y.
    
        float anguloRadianes = atan2(Y_resultante, X_resultante); //Ángulo obtenido en radianes.
        float anguloGrados = (int)(anguloRadianes * (180.0 / PI)); //Ángulo obtenido en grados.
    
        // Convertir ángulos negativos a positivos (0 a 360)
        if(anguloGrados < 0){
            anguloGrados += 360;
        }    
        // Aplicar lógica para corregir los ángulos extremos.
        if (anguloGrados > 180 && anguloGrados <= 270) {
            anguloGrados = 180;  // Si está entre 180° y 270°, se ajusta a 180°.
          } 
          else if (anguloGrados > 270 && anguloGrados <= 360) {
            anguloGrados = 0;  // Si está entre 270° y 360°, se ajusta a 0°.
        }
        return (int)anguloGrados; //Devolver el angulo, y castear a entero.
    }

}; //Fin de la clase CalcularAnguloFuerzas.

//Clase para calcular el ángulo con Desviación Estandar y cantidad de luz.
class CalcularAnguloDesviacion: public ICalculadorAngulo{
    private:
        BH1750Component& sensorLuz; // Instancia del sensor.
    public:
        //Método constructor y referencia al sensor de luz.
        CalcularAnguloDesviacion(BH1750Component& sensorLuz) : sensorLuz(sensorLuz) {}

        //Función que calcula el ángulo.
        int calcularAngulo(int L, int R, int D, int U) override{
            float media = (L + R + D + U) / 4.0; //Obtener la media
            float sumaCuadrados = pow(U - media, 2) + pow(D - media, 2) + pow(L - media, 2) + pow(R - media, 2);
            float desviacionEstandar = sqrt(sumaCuadrados/4.0); //Obtener la desviación estándar
            desviacionEstandar = constrain(desviacionEstandar, 0, 500); //Limitar la desviación entre 0 y 500.
            int desviacionMapeada = map(desviacionEstandar, 0, 500, 0, 90); //Mapeo para obtener el ángulo.
            float lux = sensorLuz.getLux();
            float intensidad; 
            if (lux > 10000 ){
                intensidad = 0.0;
            } else {
                intensidad = map(lux, 0, 10000, 20, 0); //Mapeo del valor entre 20 y 0.
            }
            intensidad = constrain(intensidad, 0, 3); //Limita la intensidad a valores entre 0 y 3.
        
            int anguloVertical = (int)(desviacionMapeada*intensidad); //Multiplica la desviación por la intesidad.
            //La intensidad ayuda a multiplicar el ángulo cuando la cantidad de luz es muy baja.
            anguloVertical = constrain(anguloVertical, 0, 90);
            return anguloVertical;
        }
}; //Fin de la clase CalcularAnguloDesviacion.

//Clase para controlar el movimiento con PID.
class ControladorPID : public IControladorServo{
    private:
        ServoComponent& servoMotor; // Instancia del servo horizontal.
        ICalculadorAngulo& calculadorAngulo; // Instancia del calculador de ángulo.
        int resolucion; //Variable para la resolución de respuestas.
        //Variables para el controlador PID.
        double kp, ki, kd;
        double integral, prevError;
        long prevTime = 0;
        int umbral;
        // Referencia a los Fotoresistores.
        // Agrega referencias a los fotoresistores
        FotoresistorComponent* fotoresistores[4]; 

    public:

        //Método constructor.
        ControladorPID(ServoComponent& servo, ICalculadorAngulo& calculador,
            int resolucion, double kp, double ki, double kd, int umbral,
            FotoresistorComponent* fr[4])
            :servoMotor(servo), calculadorAngulo(calculador), resolucion(resolucion),
            kp(kp), ki(ki), kd(kd), umbral(umbral), integral(0), prevError(0)
            {
                // Copia los punteros del array.
                for (int i = 0; i < 4; ++i) {
                fotoresistores[i] = fr[i];
                }
            }

        //Método para cambiar el calculador de ángulo.
        void setCalculadorAngulo(ICalculadorAngulo& nuevoCalculador){
            calculadorAngulo = nuevoCalculador;
        }

        //Método que realiza los cálculos PID.
        void actualizarMovimiento(int anguloDeseado) override {
            //Leer los fotoresistores.
            int L = fotoresistores[0]->getFotoresistor();
            int R = fotoresistores[1]->getFotoresistor();
            int D = fotoresistores[2]->getFotoresistor();
            int U = fotoresistores[3]->getFotoresistor();

            //Calcular el ángulo deseado.
            anguloDeseado = calculadorAngulo.calcularAngulo(L, R, D, U);
            int anguloActual = servoMotor.getAngulo();
            double error = anguloDeseado - anguloActual;
            // Verificar si el error absoluto es mayor que el umbral ---
            if (abs(error) > umbral) {  // NUEVO: Solo mover si el error > umbral
               //Calcular el valor obtenido por PID.
                double movimiento = calcularSalidaPID(error);
                movimiento = constrain(movimiento, -resolucion, resolucion); //Mantiene el resultado PID dentro de la resoluación establecida.
                 //Actualiza el movimiento del servo motor.
                anguloActual += (int)movimiento;
                anguloActual = constrain(anguloActual, 0, 180);
                servoMotor.moverServo(anguloActual);
            } //Si el error es menor, no hace nada.
        }

    private:
        //Método para actualizar un ángulo usando el controlador PID.
        void actualizarAnguloPID(int anguloDeseado){
            int anguloActual = servoMotor.getAngulo();
            double error = anguloDeseado - anguloActual;
        }

        
        //Función para calcular el valor PID.
        double calcularSalidaPID(double error){ 
            long now = millis(); //Obtiene el valor del tiempo.
            double timeChange = (double)(now - prevTime)/1000.0; //Calcula el cambio de tiempo.
            //Revisa si el arduino recién encendio 
            if (prevTime == 0) {
                prevTime = now;
                return 0.0;
            }

            double p = kp * error;
            integral += ki* error * timeChange;
            double d = kd * (error - prevError)/ timeChange;

            prevError = error;
            prevTime = now;
            return p + d;
        }
}; //Fin de la clase ControladorPID.

//Clase para controlar el movimiento con P.
class ControladorP : public IControladorServo{
    private:
        ServoComponent& servoMotor; //Referencia al servo motor.
        ICalculadorAngulo& calculadorAngulo; //Recibe la estrategia.
        int resolucion; //Resolución del movimiento.
        double kp; //Ganancia proporcional.
        int umbral; //Umbral para ejecutar o no el cambio.
        FotoresistorComponent* fotoresistores[4]; // Puntero al array
    public:
        //Método constructor.
        ControladorP(ServoComponent& servo, ICalculadorAngulo& calculador, int resolucion, double kp, int umbral, FotoresistorComponent* fr[4])
        : servoMotor(servo), calculadorAngulo(calculador), resolucion(resolucion), kp(kp), umbral(umbral) 
        {
            for (int i = 0; i < 4; ++i) 
                fotoresistores[i] = fr[i];
        }

         // Método para cambiar la estrategia de cálculo
        void setCalculadorAngulo(ICalculadorAngulo& nuevoCalculador) {
        calculadorAngulo = nuevoCalculador;
        }

        //Método para actualizar el movimiento.
        void actualizarMovimiento(int anguloDeseado) override {
            //Leer los fotoresistores.
            int L = fotoresistores[0]->getFotoresistor();
            int R = fotoresistores[1]->getFotoresistor();
            int D = fotoresistores[2]->getFotoresistor();
            int U = fotoresistores[3]->getFotoresistor();

            //Uso de la estrategia escogida.
            anguloDeseado = calculadorAngulo.calcularAngulo(L,R,D,U);
            int anguloActual = servoMotor.getAngulo();
            double error = anguloDeseado - anguloActual;

            //Control de movimiento con el Umbral.
            if (abs(error) > umbral) {
                double movimiento = kp * error;
                movimiento = constrain(movimiento, -resolucion, resolucion);
                anguloActual += (int)movimiento;
                anguloActual = constrain(anguloActual, 0, 180); 
                servoMotor.moverServo(anguloActual);
            }
        
        }
}; //Fin de la clase ControladorP


//----------------------------//
//------- Observadores -------//
//----------------------------//

//Observador para recibir los datos y unirlos en un string.
class MensajeObservador : public Observador{
    private:
        MensajeBuilder builder;
    public:

        // Recibe la notificación del sensor.
        void update(const String& componente, const String& valor) override {
            builder.agregar(componente, valor); // Delega la construcción al builder.
        }

        // Método para obtener el mensaje completo (ya formateado).
        Mensaje getMensaje() {
            return builder.construir(); // Construye y devuelve el Mensaje.
        }
        // AGREGAMOS LA FUNCION LIMPIAR
        void limpiar(){
            builder.limpiar();
        }
}; //Fin de la clase mensajeObservador.

// Observador para imprimir los datos de los componentes en el monitor serial.
class SerialMonitorObservador : public Observador {

    private:
    MensajeObservador& mensajeObservador; // Referencia a MensajeObservador

    public: 
    // Constructor que toma la referencia
    SerialMonitorObservador(MensajeObservador& mensaje) : mensajeObservador(mensaje)  {}


    void update(const String& /*sensor*/, const String& /*valor*/) override {
        // Usamos la referencia a MensajeObservador para obtener el mensaje completo
       Serial.println(mensajeObservador.getMensaje().getMensaje());
    }
}; //Fin de la clase SerialMonitorObservador.



//--------------------------//
//----- Configuración ------//
//--------------------------//

BH1750Component sensorLuz; // Instancia del sensor.
TemperaturaComponent sensorTemperatura(7); // Instancia del sensor de temperatura.
HumedadComponent sensorHumedad(7);
FotoresistorComponent fotoresistor0(A0); //Instancia del fotoresistor.
FotoresistorComponent fotoresistor1(A1); //Instancia del fotoresistor.
FotoresistorComponent fotoresistor2(A2); //Instancia del fotoresistor.
FotoresistorComponent fotoresistor3(A3); //Instancia del fotoresistor.
ServoComponent servoHorizontal(9); // Instancia del servo.
ServoComponent servoVertical(10); // Instancia del servo.
MensajeObservador mensajeObservador; // Instancia del observador.
CalcularAnguloFuerzas calculadorFuerzas; //Instancia para calcular el ángulo del servo horizontal.
CalcularAnguloDesviacion calculadorDesviacion(sensorLuz); //Instancia para calcular el ángulo del servo vertical.
Scheduler scheduler; // Instancia del TaskScheduler.


// Arreglo de punteros a IInicializador
IInicializador* initObjects[] = {
  &sensorLuz,
  &sensorTemperatura,
  &sensorHumedad,
  &fotoresistor0,
  &fotoresistor1,
  &fotoresistor2,
  &fotoresistor3,
  &servoHorizontal,
  &servoVertical
};

// Arreglo de punteros a Sujeto
Sujeto* sujetoObjects[] = {
  &sensorLuz,
  &sensorTemperatura,
  &sensorHumedad,
  &fotoresistor0,
  &fotoresistor1,
  &fotoresistor2,
  &fotoresistor3,
  &servoHorizontal,
  &servoVertical
};
// Crear el array de punteros a FotoresistorComponent
FotoresistorComponent* fotoresistores[] = {&fotoresistor0, &fotoresistor1, &fotoresistor2, &fotoresistor3};

ControladorPID controladorPID(servoHorizontal, calculadorFuerzas, 50, 1, 0.1, 0.1, 15,
    fotoresistores); // Instancia del controlador PID.
ControladorP controladorP(servoVertical, calculadorDesviacion, 50, 1.5, 10,
    fotoresistores); //Instancia del controlador P.
IControladorServo* controladorHorizontal = &controladorPID;
IControladorServo* controladorVertical = &controladorP;

Task tareaLeerSensor(1000, TASK_FOREVER, []() {
    // Iterar sobre el arreglo y llamar al método que notifica *a cada componente*.
    for (int i = 0; i < sizeof(sujetoObjects) / sizeof(sujetoObjects[0]); i++) {
        sujetoObjects[i]->componenteNotificar();
    }
    // *Después* de notificar a todos, obtenemos e imprimimos el mensaje *completo*.
    Serial.println(mensajeObservador.getMensaje().getMensaje());
    mensajeObservador.limpiar(); // ¡Limpiamos DESPUÉS de imprimir!
    tareaLeerSensor.enableDelayed(); // Reactiva la tarea
});


// Tarea para el movimiento del servo usando el ControladorPID
Task tareaMoverPanel(1000, TASK_FOREVER, []() {

    //Actualizar el movimiento del servo.
    controladorHorizontal->actualizarMovimiento(0);
    controladorVertical->actualizarMovimiento(0);
    
});


//-------------------------//
//----- Función Setup -----//
//-------------------------//
#line 639 "C:\\Users\\ferx666g\\Desktop\\Software\\Trabajo de Titulación\\ProyectoTitulacion\\ArduinoVSCode\\ControladorA1\\ControladorA1.ino"
void setup();
#line 660 "C:\\Users\\ferx666g\\Desktop\\Software\\Trabajo de Titulación\\ProyectoTitulacion\\ArduinoVSCode\\ControladorA1\\ControladorA1.ino"
void loop();
#line 639 "C:\\Users\\ferx666g\\Desktop\\Software\\Trabajo de Titulación\\ProyectoTitulacion\\ArduinoVSCode\\ControladorA1\\ControladorA1.ino"
void setup() {
    Wire.begin();
    Serial.begin(115200);


    // Iterar sobre el arreglo y llamar a init()
    for (int i = 0; i < sizeof(initObjects) / sizeof(initObjects[0]); i++) {
      initObjects[i]->init();
    }

    for (int i = 0; i < sizeof(sujetoObjects) / sizeof(sujetoObjects[0]); i++) {
        sujetoObjects[i]->agregarObservador(&mensajeObservador);
    }

    
    scheduler.addTask(tareaLeerSensor);
    tareaLeerSensor.enable();
    scheduler.addTask(tareaMoverPanel);
    tareaMoverPanel.enable();
}

void loop() {
    scheduler.execute();
}

