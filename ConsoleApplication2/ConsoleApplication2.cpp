#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <map>
#include <sstream>
#include <sqlite3.h> 

using namespace std;

// -------------------------------------------------------------------
// ESTRUCTURA DE DATOS Y ENUMS
// -------------------------------------------------------------------

// Roles disponibles en el sistema
enum RolUsuario { ADMINISTRADOR, CAJERO };

// Estructura simplificada para manejar la data recuperada de la BD
struct Producto {
    int id;
    string nombre;
    int stockActual;
    int stockMinimo;
};

// -------------------------------------------------------------------
// CLASE 1: Usuario (Autenticación y Roles)
// -------------------------------------------------------------------

class Usuario {
private:
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
// CLASE 2: Inventario (Logica de Negocio y SQL)
// -------------------------------------------------------------------

class Inventario {
private:
    sqlite3* db; // Puntero a la conexion de la BD

    // Callback estático para procesar los resultados de SELECT
    static int callbackMostrar(void* data, int argc, char** argv, char** azColName) {
        // argc: numero de columnas, argv: valores, azColName: nombres de las columnas

        // Convertir datos de la BD a la estructura Producto para la logica visual
        int id = std::stoi(argv[0] ? argv[0] : "0");
        string nombre = argv[1] ? argv[1] : "N/A";
        int stock = std::stoi(argv[2] ? argv[2] : "0");
        int stockMin = std::stoi(argv[3] ? argv[3] : "0");

        string estado;
        if (stock <= stockMin) {
            estado = "🔴 BAJO STOCK (Reponer)"; // Futuro color ROJO
        }
        else if (stock < (stockMin * 2)) {
            estado = "🟡 ALERTA"; // Futuro color AMARILLO
        }
        else {
            estado = "🟢 OK"; // Futuro color VERDE
        }

        // Imprimir la fila formateada
        cout << left << setw(5) << id
            << setw(25) << nombre
            << setw(15) << stock
            << setw(10) << stockMin
            << estado << "\n";

        return 0; // Continuar leyendo la siguiente fila
    }

    // Funcion auxiliar para crear la tabla de productos
    void crearTabla() {
        char* errorMsg;
        const char* sql =
            "CREATE TABLE IF NOT EXISTS PRODUCTOS("
            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
            "NOMBRE TEXT NOT NULL,"
            "STOCK_ACTUAL INTEGER NOT NULL,"
            "STOCK_MINIMO INTEGER NOT NULL,"
            "PRECIO_UNITARIO REAL);";

        int rc = sqlite3_exec(db, sql, NULL, 0, &errorMsg);

        if (rc != SQLITE_OK) {
            cerr << "Error al crear tabla: " << errorMsg << endl;
            sqlite3_free(errorMsg);
        }
        else {
            // Inicializar con data si la tabla esta vacia (opcional)
            const char* check_sql = "SELECT COUNT(*) FROM PRODUCTOS;";
            int count = 0;
            auto count_callback = [](void* data, int argc, char** argv, char** azColName) -> int {
                *(int*)data = std::stoi(argv[0]);
                return 0;
                };
            sqlite3_exec(db, check_sql, count_callback, &count, NULL);

            if (count == 0) {
                // Insertar datos de ejemplo si la BD es nueva
                const char* insert_sql =
                    "INSERT INTO PRODUCTOS (NOMBRE, STOCK_ACTUAL, STOCK_MINIMO, PRECIO_UNITARIO) VALUES ('Monitor 27', 15, 5, 250.00);"
                    "INSERT INTO PRODUCTOS (NOMBRE, STOCK_ACTUAL, STOCK_MINIMO, PRECIO_UNITARIO) VALUES ('Teclado Mecanico', 3, 10, 80.00);"
                    "INSERT INTO PRODUCTOS (NOMBRE, STOCK_ACTUAL, STOCK_MINIMO, PRECIO_UNITARIO) VALUES ('Mouse Optico', 50, 20, 15.00);";
                sqlite3_exec(db, insert_sql, NULL, 0, NULL);
            }
        }
    }

public:
    Inventario() {
        // ⭐ Conexión a la base de datos (crea el archivo inventario.db si no existe)
        int rc = sqlite3_open("inventario.db", &db);

        if (rc) {
            cerr << "❌ No se puede abrir la BD: " << sqlite3_errmsg(db) << endl;
            db = nullptr; // Marcar como fallido
        }
        else {
            cout << "Conexion a la BD SQLite exitosa." << endl;
            crearTabla();
        }
    }

    ~Inventario() {
        if (db) {
            sqlite3_close(db); // Cerrar la conexión
        }
    }

    void mostrarResumen() const {
        if (!db) { cerr << "❌ Error: BD no conectada." << endl; return; }

        cout << "\n--- RESUMEN DE INVENTARIO (Desde SQL) ---\n";
        cout << left << setw(5) << "ID"
            << setw(25) << "Nombre"
            << setw(15) << "Stock Actual"
            << setw(10) << "Min."
            << "ESTADO VISUAL\n";
        cout << "----------------------------------------------------------------\n";

        const char* sql = "SELECT ID, NOMBRE, STOCK_ACTUAL, STOCK_MINIMO FROM PRODUCTOS ORDER BY ID ASC;";

        sqlite3_exec(db, sql, callbackMostrar, NULL, NULL);
    }

    // Registra una salida de producto (UPDATE)
    void registrarSalida(int id, int cantidad) {
        if (!db) { cerr << "❌ Error: BD no conectada." << endl; return; }

        // Consulta para actualizar el stock (UPDATE)
        stringstream sql;
        sql << "UPDATE PRODUCTOS SET STOCK_ACTUAL = STOCK_ACTUAL - " << cantidad
            << " WHERE ID = " << id << " AND STOCK_ACTUAL >= " << cantidad << ";";

        char* errorMsg;
        int rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &errorMsg);

        if (rc != SQLITE_OK) {
            cerr << "❌ Error al actualizar: " << errorMsg << endl;
            sqlite3_free(errorMsg);
        }
        else {
            // Verificar si alguna fila fue modificada (para saber si habia stock)
            if (sqlite3_changes(db) > 0) {
                cout << "\n✅ Salida de " << cantidad << " unidades registrada en BD (ID: " << id << ").\n";
            }
            else {
                cout << "\n❌ Error: No se encontro el producto o el stock es insuficiente.\n";
            }
        }
    }

    // Ingreso de nuevo producto (INSERT)
    void ingresarNuevoProducto(const string& nombre, int stockInicial, int stockMin, double precio) {
        if (!db) { cerr << "❌ Error: BD no conectada." << endl; return; }

        // Consulta para insertar un nuevo producto (INSERT)
        stringstream sql;
        sql << "INSERT INTO PRODUCTOS (NOMBRE, STOCK_ACTUAL, STOCK_MINIMO, PRECIO_UNITARIO) VALUES ('"
            << nombre << "', " << stockInicial << ", " << stockMin << ", " << fixed << setprecision(2) << precio << ");";

        char* errorMsg;
        int rc = sqlite3_exec(db, sql.str().c_str(), NULL, 0, &errorMsg);

        if (rc != SQLITE_OK) {
            cerr << "❌ Error al insertar: " << errorMsg << endl;
            sqlite3_free(errorMsg);
        }
        else {
            cout << "⭐ Producto ingresado con éxito en la BD.\n";
        }
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
            cout << "     MENU PRINCIPAL INVENTARIO\n";
            cout << "================================\n";
            cout << "1. Ver Resumen Visual de Inventario\n";
            cout << "2. Registrar Venta (Salida)\n";

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
                case 4: cout << "➡️ Aqui va la implementacion grafica con librerias como Qt Charts. 📊\n"; break;
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
                case 3: cout << "➡️ Aqui va la implementacion grafica con librerias como Qt Charts. 📊\n"; break;
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
