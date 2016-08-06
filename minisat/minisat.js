

newSolverState = Module.cwrap('newSolverState', 'number', ['string']);
freeSolverState = Module.cwrap('freeSolverState', null, ['number']);
c_solverStateAddClause = Module.cwrap('solverStateAddClause', null, ['number', 'number']);
c_solverStateSolve = Module.cwrap('solverStateSolve', 'number', ['number', 'number']);

//code pulled from http://kapadia.github.io/emscripten/2013/09/13/emscripten-pointers-and-pointers.html
function solverStateAddClause(solver, arr) {
    var data = new Int32Array(arr);
    var nDataBytes = data.length * data.BYTES_PER_ELEMENT;
    var dataPtr = Module._malloc(nDataBytes);
    var dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, nDataBytes); 
    dataHeap.set(new Uint8Array(data.buffer));
    c_solverStateAddClause(solver, dataHeap.buffer);
}

function solverStateSolve(solver, out) {
    var data = new Int32Array(arr);
    var nDataBytes = data.length * data.BYTES_PER_ELEMENT;
    var dataPtr = Module._malloc(nDataBytes);
    var dataHeap = new Uint8Array(Module.HEAPU8.buffer, dataPtr, nDataBytes);
    dataHeap.set(new Uint8Array(data.buffer));
    var ret = c_solverStateSolve(solver, dataHeap.buffer);
    //so now somehow we need to translate back so we can add things to out
    //yea this needs to be added
}
