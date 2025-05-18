#include <iostream>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
// No need for namespace_zero_generated.h if UA_NS0ID_ constants are in server.h

#include <csignal> // Still good for understanding, though runUntilInterrupt handles SIGINT/SIGTERM
#include <cmath>   // For sin function

// Global variable for the server pointer, so stopHandler can potentially access it if needed
// (though runUntilInterrupt handles the shutdown gracefully based on signals)
// static UA_Server *g_server = NULL; // Not strictly needed if runUntilInterrupt is used

// volatile UA_Boolean running = true; // Not directly used by the main loop if using runUntilInterrupt

// Signal handler - UA_Server_runUntilInterrupt will catch SIGINT/SIGTERM
// This handler is mostly for our logging or if we wanted to do custom cleanup
// before the library shuts down the server.
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Signal %d received, UA_Server_runUntilInterrupt will stop the server.", sig);
    // If you had a global `running` flag, you could set it here,
    // but runUntilInterrupt makes it less necessary for the loop itself.
    // running = false;
    // If you had a global server pointer, you could potentially call UA_Server_shutdown(g_server)
    // but runUntilInterrupt does this.
}

static UA_StatusCode
readValueCallback(UA_Server* server,
    const UA_NodeId* sessionId, void* sessionContext,
    const UA_NodeId* nodeId, void* nodeContext,
    UA_Boolean sourceTimeStamp, const UA_NumericRange* range,
    UA_DataValue* dataValue) {

    static double counter = 0.0;
    counter += 0.5;
    if (counter > 1000.0) {
        counter = 0.0;
    }
    UA_Variant_setScalarCopy(&dataValue->value, &counter, &UA_TYPES[UA_TYPES_DOUBLE]);
    dataValue->hasValue = true;
    if (sourceTimeStamp) {
        dataValue->hasSourceTimestamp = true;
        dataValue->sourceTimestamp = UA_DateTime_now();
    }
    return UA_STATUSCODE_GOOD;
}

static void
addDynamicVariable(UA_Server* server) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    // For dynamic data sources, we don't set attr.value directly.
    // The value comes from the callback.

    // Using _ALLOC versions for strings is good practice, but requires _clear later.
    // Let's stick to const_cast for simplicity if memory management is not an immediate concern,
    // but be aware of the _ALLOC alternatives from the working example.
    attr.displayName = UA_LOCALIZEDTEXT(const_cast<char*>("en-US"), const_cast<char*>("DynamicData"));
    attr.description = UA_LOCALIZEDTEXT(const_cast<char*>("en-US"), const_cast<char*>("A variable that updates dynamically."));
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_NodeId dynamicVariableNodeId = UA_NODEID_STRING(1, const_cast<char*>("dynamic.double.value"));
    UA_QualifiedName dynamicVariableName = UA_QUALIFIEDNAME(1, const_cast<char*>("DynamicDoubleValue"));
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeDefinition = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource dynamicDataSource;
    dynamicDataSource.read = readValueCallback;
    dynamicDataSource.write = NULL;

    UA_Server_addDataSourceVariableNode(server,
        dynamicVariableNodeId, parentNodeId, referenceTypeId,
        dynamicVariableName, variableTypeDefinition, attr,
        dynamicDataSource, NULL, NULL);

    // No UA_VariableAttributes_clear(&attr) needed here if not using _ALLOC for its members
    // No UA_NodeId_clear needed if not using _ALLOC
    // No UA_QualifiedName_clear needed if not using _ALLOC

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Added DynamicData variable node (ns=1;s=dynamic.double.value)");
}


int main(int argc, char* argv[]) {
    // It's still good to register signal handlers for awareness,
    // even if runUntilInterrupt handles the core logic.
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server* server = UA_Server_new();
    if (!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Failed to create server instance!");
        return EXIT_FAILURE;
    }
    // g_server = server; // If you needed global access in stopHandler

    UA_ServerConfig* config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    // No need to call UA_ServerConfig_setCustomHostname if default behavior (listen on all) is desired
    // and works with UA_Server_runUntilInterrupt.

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "OPC UA Server configured with defaults.");
    addDynamicVariable(server);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Starting server with UA_Server_runUntilInterrupt. Press Ctrl+C to exit.");

    // This call will block until SIGINT or SIGTERM is received.
    // It handles the server's main event loop and network listening internally.
    UA_StatusCode retval = UA_Server_runUntilInterrupt(server);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Server shutting down...");
    UA_Server_delete(server);
    // g_server = NULL;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Server stopped.");

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}