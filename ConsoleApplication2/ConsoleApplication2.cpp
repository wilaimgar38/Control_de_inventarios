#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <map>
#include <sstream>
#include <algorithm> // Necesario para std::find_if

using namespace std;

// -------------------------------------------------------------------
// ESTRUCTURA DE DATOS Y ENUMS
// -------------------------------------------------------------------

// Roles disponibles en el sistema
enum RolUsuario { ADMINISTRADOR, CAJERO };

// Estructura para manejar la data de los productos
struct Producto {
    int id;
    string nombre;
    int stockActual;
    int stockMinimo;
    double precioUnitario; // Se mantuvo el precio para futura funcionalidad
};

// Contador global para asignar IDs únicos automáticamente
int siguienteID = 1;

// -------------------------------------------------------------------
// CLASE 1: Usuario (Autenticación y Roles)
// -------------------------------------------------------------------

class Usuario {
private:
    // Credenciales y roles en memoria
    map<string, pair<string, RolUsuario>> credenciales = {
        {"admin",   {"pass123", ADMINISTRADOR}},
        {"caja",    {"venta456", CAJERO}}
    };
    RolUsuario rolActual;

public:
    RolUsuario getRolActual() const {
        return rolActual;
    }

    bool iniciarSesion() {
        string user, pass;
        cout << "\n--- INICIO DE SESION ---\n";
        cout << "Usuario: ";
        cin >> user;
        cout << "Clave: ";
        cin >> pass;

        if (credenciales.count(user) && credenciales[user].first == pass) {
            rolActual = credenciales[user].second;
            cout << "\n✅ ¡Acceso concedido! Bienvenido/a, " << user << ".\n";
            return true;
        }
        else {
            cout << "\n❌ Error de autenticacion. Intente de nuevo.\n";
            return false;
        }
    }
};

// -------------------------------------------------------------------
// CLASE 2: Inventario (Logica de Negocio y Data en Memoria)
// -------------------------------------------------------------------

class Inventario {
private:
    // Almacenamiento de datos en memoria (reemplaza a SQLite)
    vector<Producto> productos;

    // Función auxiliar para inicializar el vector con datos de ejemplo
    void inicializarData() {
        productos.push_back({ siguienteID++, "Monitor 27", 15, 5, 250.00 });
        productos.push_back({ siguienteID++, "Teclado Mecanico", 3, 10, 80.00 }); // Estará en BAJO STOCK
        productos.push_back({ siguienteID++, "Mouse Optico", 50, 20, 15.00 });
        productos.push_back({ siguienteID++, "Webcam HD", 15, 8, 45.00 }); // Estará en ALERTA
    }

public:
    Inventario() {
        cout << "Inicializando sistema de Inventario en Memoria (sin BD)." << endl;
        inicializarData();
    }

    // El destructor ya no cierra la BD, solo limpia recursos (opcional en este caso)
    ~Inventario() {}

    void mostrarResumen() const {
        cout << "\n--- RESUMEN DE INVENTARIO (Data en Memoria) ---\n";
        cout << left << setw(5) << "ID"
            << setw(25) << "Nombre"
            << setw(15) << "Stock Actual"
            << setw(10) << "Min."
            << "ESTADO VISUAL\n";
        cout << "----------------------------------------------------------------\n";

        for (const auto& p : productos) {
            string estado;
            // Lógica de estado visual (misma que el callback anterior)
            if (p.stockActual <= p.stockMinimo) {
                estado = "🔴 BAJO STOCK (Reponer)";
            }
            else if (p.stockActual < (p.stockMinimo * 2)) {
                estado = "🟡 ALERTA";
            }
            else {
                estado = "🟢 OK";
            }

            // Imprimir la fila formateada
            cout << left << setw(5) << p.id
                << setw(25) << p.nombre
                << setw(15) << p.stockActual
                << setw(10) << p.stockMinimo
                << estado << "\n";
        }
    }

    // Registra una salida de producto (UPDATE) en memoria
    void registrarSalida(int id, int cantidad) {
        // Buscar el producto por ID usando std::find_if
        auto it = std::find_if(productos.begin(), productos.end(),
            [id](const Producto& p) { return p.id == id; });

        if (it != productos.end()) {
            // Producto encontrado
            if (it->stockActual >= cantidad) {
                it->stockActual -= cantidad; // Reducir stock
                cout << "\n✅ Salida de " << cantidad << " unidades registrada (ID: " << id << ", Stock restante: " << it->stockActual << ").\n";
            }
            else {
                cout << "\n❌ Error: El stock actual (" << it->stockActual << ") es insuficiente para la venta de " << cantidad << " unidades.\n";
            }
        }
        else {
            cout << "\n❌ Error: No se encontro el producto con ID: " << id << ".\n";
        }
    }

    // Ingreso de nuevo producto (INSERT) en memoria
    void ingresarNuevoProducto(const string& nombre, int stockInicial, int stockMin, double precio) {
        // Crear un nuevo producto y asignarle el siguiente ID disponible
        Producto nuevo = { siguienteID++, nombre, stockInicial, stockMin, precio };
        productos.push_back(nuevo);

        cout << "⭐ Producto '" << nombre << "' ingresado con éxito (ID: " << nuevo.id << ").\n";
    }
};

// -------------------------------------------------------------------
// CLASE 3: AppControlador (Flujo del Programa con Permisos)
// -------------------------------------------------------------------

class AppControlador {
private:
    Usuario userManager;
    Inventario invManager;
    bool autenticado = false;

    void solicitarDatosIngreso() {
        string nombre;
        int stockInicial, stockMin;
        double precio;

        cout << "\n--- INGRESO DE NUEVO PRODUCTO ---\n";
        cout << "Nombre del producto: ";
        cin.ignore();
        getline(cin, nombre);

        cout << "Stock Inicial: ";
        if (!(cin >> stockInicial)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Entrada invalida.\n"; return; }

        cout << "Stock Minimo de Alerta: ";
        if (!(cin >> stockMin)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Entrada invalida.\n"; return; }

        cout << "Precio Unitario: ";
        if (!(cin >> precio)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Entrada invalida.\n"; return; }

        invManager.ingresarNuevoProducto(nombre, stockInicial, stockMin, precio);
    }

    void mostrarMenu() {
        int opcion;
        do {
            cout << "\n================================\n";
            cout << "      MENU PRINCIPAL INVENTARIO\n";
            cout << "================================\n";
            cout << "1. Ver Resumen Visual de Inventario\n";
            cout << "2. Registrar Venta (Salida)\n";

            // El menú se adapta al rol (ADMINISTRADOR vs CAJERO)
            if (userManager.getRolActual() == ADMINISTRADOR) {
                cout << "3. ⭐ Ingreso de NUEVO Producto (Admin)\n";
                cout << "4. (Futuro) Ver Graficos Avanzados 📊\n";
                cout << "5. Cerrar Sesion / Salir\n";
            }
            else {
                cout << "3. (Futuro) Ver Graficos Avanzados 📊\n";
                cout << "4. Cerrar Sesion / Salir\n";
            }

            cout << "Seleccione una opcion: ";
            if (!(cin >> opcion)) {
                cin.clear(); cin.ignore(10000, '\n'); opcion = 0;
            }

            // Manejo de opciones basado en el rol
            if (userManager.getRolActual() == ADMINISTRADOR) {
                switch (opcion) {
                case 1: invManager.mostrarResumen(); break;
                case 2: { // Registrar Venta
                    int id, cant;
                    cout << "Ingrese ID del producto: ";
                    if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Entrada invalida.\n"; break; }
                    cout << "Ingrese cantidad vendida: ";
                    if (!(cin >> cant)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Entrada invalida.\n"; break; }
                    invManager.registrarSalida(id, cant);
                    break;
                }
                case 3: solicitarDatosIngreso(); break;
                case 4: cout << "➡️ Esta funcionalidad usaria librerias graficas externas. 📊\n"; break;
                case 5: cout << "Cerrando sesion. ¡Vuelva pronto!\n"; autenticado = false; break;
                default: cout << "Opcion invalida. Intente de nuevo.\n";
                }
            }
            else { // Menú para Cajero
                switch (opcion) {
                case 1: invManager.mostrarResumen(); break;
                case 2: { // Registrar Venta
                    int id, cant;
                    cout << "Ingrese ID del producto: ";
                    if (!(cin >> id)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Entrada invalida.\n"; break; }
                    cout << "Ingrese cantidad vendida: ";
                    if (!(cin >> cant)) { cin.clear(); cin.ignore(10000, '\n'); cout << "Entrada invalida.\n"; break; }
                    invManager.registrarSalida(id, cant);
                    break;
                }
                case 3: cout << "➡️ Esta funcionalidad usaria librerias graficas externas. 📊\n"; break;
                case 4: cout << "Cerrando sesion. ¡Vuelva pronto!\n"; autenticado = false; break;
                default: cout << "Opcion invalida. Intente de nuevo.\n";
                }
            }
        } while (autenticado);
    }

public:
    void iniciarAplicacion() {
        if (userManager.iniciarSesion()) {
            autenticado = true;
            mostrarMenu();
        }
        else {
            cout << "\nLa aplicacion se ha cerrado.\n";
        }
    }
};

// -------------------------------------------------------------------
// FUNCION PRINCIPAL (MAIN)
// -------------------------------------------------------------------

int main() {
    AppControlador app;
    app.iniciarAplicacion();
    return 0;
}
