#include <wasmedge/wasmedge.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>


WasmEdge_ConfigureContext *ConfCxt;
WasmEdge_VMContext *VMCxt;

void VMInit() {
    ConfCxt = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(ConfCxt, WasmEdge_HostRegistration_Wasi);
    VMCxt = WasmEdge_VMCreate(ConfCxt, nullptr);
}

void VMDeallocation() {
    WasmEdge_VMDelete(VMCxt);
    WasmEdge_ConfigureDelete(ConfCxt);
}

WasmEdge_Result VMLoadWasmFile(const char* WasmPath) {
    // load
    WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VMCxt, WasmPath);
    if (!WasmEdge_ResultOK(Res)) {
        printf("Loading phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
        return Res;
    }

    // validate
    Res = WasmEdge_VMValidate(VMCxt);
    if (!WasmEdge_ResultOK(Res)) {
        printf("Validation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
        return Res;
    }

    // instantiate
    Res = WasmEdge_VMInstantiate(VMCxt);
    if (!WasmEdge_ResultOK(Res)) {
        printf("Instantiation phase failed: %s\n", WasmEdge_ResultGetMessage(Res));
        return Res;
    }
    return Res;
}

// if there is only one function in WASM file, run it
WasmEdge_Result VMRunFunction(int Argc, const char* Argv[]) {
    WasmEdge_String Names[1];
    const WasmEdge_FunctionTypeContext* FuncTypes[1];
    uint32_t FuncListSize = WasmEdge_VMGetFunctionList(VMCxt, Names, FuncTypes, 1);
    if (FuncListSize > 1) {
        printf("more than one function in wasm\n");
        return WasmEdge_Result_Success;
    }

    // get param length and return length
    uint32_t ParamLength = WasmEdge_FunctionTypeGetParametersLength(FuncTypes[0]);
    uint32_t ReturnLength = WasmEdge_FunctionTypeGetReturnsLength(FuncTypes[0]);

    auto* Params = new WasmEdge_Value[ParamLength]();
    auto* Returns = new WasmEdge_Value[ReturnLength];

    for (int i = 0; i < Argc; i++) {
        // only support i32 type
        WasmEdge_Value Val =  WasmEdge_ValueGenI32(atoi(Argv[i]));
        Params[i] = Val;
        // todo support float
    }

    /* Run the WASM function from file. */
    WasmEdge_Result Res = WasmEdge_VMExecute(VMCxt, Names[0], Params, ParamLength, Returns, ReturnLength);
    if (WasmEdge_ResultOK(Res)) {
        if (ReturnLength >= 0) {
            printf("%d\n", WasmEdge_ValueGetI32(Returns[0]));
            // todo support float
        }
    } else {
        printf("Error message: %s\n", WasmEdge_ResultGetMessage(Res));
    }

    delete[] Params;
    delete[] Returns;
    return Res;
}

int main(int Argc, const char* Argv[]) {
    // parse arguments
    if (Argc <= 1) {
        printf("tool [version] [run] [wasm path] [arguments]\n");
        return 1;
    }
    if (!strcmp(Argv[1], "version")) {
        // ignore any other options
        printf("%s\n", WasmEdge_VersionGet());
        return 0;
    }

    const char* WasmPath;
    int WasmPathIdx = 0;
    if (strcmp(Argv[1], "run") != 0) {
        // without run option
        WasmPathIdx = 1;
    } else if (Argc >= 3){
        WasmPathIdx = 2;
    }
    WasmPath = WasmPathIdx > 0 ? Argv[WasmPathIdx] : nullptr;
    if (!WasmPath) {
        printf("wasm path is required\n");
        return 1;
    }

    VMInit();

    WasmEdge_Result Res;

    Res = VMLoadWasmFile(WasmPath);
    if (!WasmEdge_ResultOK(Res)) {
        return 1;
    }

    Res = VMRunFunction(Argc - WasmPathIdx - 1, &Argv[WasmPathIdx + 1]);
    if (!WasmEdge_ResultOK(Res)) {
        return 1;
    }

    VMDeallocation();

    return 0;
}