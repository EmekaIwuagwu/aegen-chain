"use strict";
/*
 * ATTENTION: An "eval-source-map" devtool has been used.
 * This devtool is neither made for production nor for readable output files.
 * It uses "eval()" calls to create a separate source file with attached SourceMaps in the browser devtools.
 * If you are trying to read the output file, select a different devtool (https://webpack.js.org/configuration/devtool/)
 * or disable the default devtool with "devtool: false".
 * If you are looking for production-ready output files, see mode: "production" (https://webpack.js.org/configuration/mode/).
 */
exports.id = "vendor-chunks/available-typed-arrays";
exports.ids = ["vendor-chunks/available-typed-arrays"];
exports.modules = {

/***/ "(ssr)/./node_modules/available-typed-arrays/index.js":
/*!******************************************************!*\
  !*** ./node_modules/available-typed-arrays/index.js ***!
  \******************************************************/
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

eval("\nvar possibleNames = __webpack_require__(/*! possible-typed-array-names */ \"(ssr)/./node_modules/possible-typed-array-names/index.js\");\nvar g = typeof globalThis === \"undefined\" ? global : globalThis;\n/** @type {import('.')} */ module.exports = function availableTypedArrays() {\n    var /** @type {ReturnType<typeof availableTypedArrays>} */ out = [];\n    for(var i = 0; i < possibleNames.length; i++){\n        if (typeof g[possibleNames[i]] === \"function\") {\n            // @ts-expect-error\n            out[out.length] = possibleNames[i];\n        }\n    }\n    return out;\n};\n//# sourceURL=[module]\n//# sourceMappingURL=data:application/json;charset=utf-8;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiKHNzcikvLi9ub2RlX21vZHVsZXMvYXZhaWxhYmxlLXR5cGVkLWFycmF5cy9pbmRleC5qcyIsIm1hcHBpbmdzIjoiQUFBQTtBQUVBLElBQUlBLGdCQUFnQkMsbUJBQU9BLENBQUM7QUFFNUIsSUFBSUMsSUFBSSxPQUFPQyxlQUFlLGNBQWNDLFNBQVNEO0FBRXJELHdCQUF3QixHQUN4QkUsT0FBT0MsT0FBTyxHQUFHLFNBQVNDO0lBQ3pCLElBQUksb0RBQW9ELEdBQUdDLE1BQU0sRUFBRTtJQUNuRSxJQUFLLElBQUlDLElBQUksR0FBR0EsSUFBSVQsY0FBY1UsTUFBTSxFQUFFRCxJQUFLO1FBQzlDLElBQUksT0FBT1AsQ0FBQyxDQUFDRixhQUFhLENBQUNTLEVBQUUsQ0FBQyxLQUFLLFlBQVk7WUFDOUMsbUJBQW1CO1lBQ25CRCxHQUFHLENBQUNBLElBQUlFLE1BQU0sQ0FBQyxHQUFHVixhQUFhLENBQUNTLEVBQUU7UUFDbkM7SUFDRDtJQUNBLE9BQU9EO0FBQ1IiLCJzb3VyY2VzIjpbIndlYnBhY2s6Ly9hZWdlbi13YWxsZXQvLi9ub2RlX21vZHVsZXMvYXZhaWxhYmxlLXR5cGVkLWFycmF5cy9pbmRleC5qcz8xNzVmIl0sInNvdXJjZXNDb250ZW50IjpbIid1c2Ugc3RyaWN0JztcblxudmFyIHBvc3NpYmxlTmFtZXMgPSByZXF1aXJlKCdwb3NzaWJsZS10eXBlZC1hcnJheS1uYW1lcycpO1xuXG52YXIgZyA9IHR5cGVvZiBnbG9iYWxUaGlzID09PSAndW5kZWZpbmVkJyA/IGdsb2JhbCA6IGdsb2JhbFRoaXM7XG5cbi8qKiBAdHlwZSB7aW1wb3J0KCcuJyl9ICovXG5tb2R1bGUuZXhwb3J0cyA9IGZ1bmN0aW9uIGF2YWlsYWJsZVR5cGVkQXJyYXlzKCkge1xuXHR2YXIgLyoqIEB0eXBlIHtSZXR1cm5UeXBlPHR5cGVvZiBhdmFpbGFibGVUeXBlZEFycmF5cz59ICovIG91dCA9IFtdO1xuXHRmb3IgKHZhciBpID0gMDsgaSA8IHBvc3NpYmxlTmFtZXMubGVuZ3RoOyBpKyspIHtcblx0XHRpZiAodHlwZW9mIGdbcG9zc2libGVOYW1lc1tpXV0gPT09ICdmdW5jdGlvbicpIHtcblx0XHRcdC8vIEB0cy1leHBlY3QtZXJyb3Jcblx0XHRcdG91dFtvdXQubGVuZ3RoXSA9IHBvc3NpYmxlTmFtZXNbaV07XG5cdFx0fVxuXHR9XG5cdHJldHVybiBvdXQ7XG59O1xuIl0sIm5hbWVzIjpbInBvc3NpYmxlTmFtZXMiLCJyZXF1aXJlIiwiZyIsImdsb2JhbFRoaXMiLCJnbG9iYWwiLCJtb2R1bGUiLCJleHBvcnRzIiwiYXZhaWxhYmxlVHlwZWRBcnJheXMiLCJvdXQiLCJpIiwibGVuZ3RoIl0sInNvdXJjZVJvb3QiOiIifQ==\n//# sourceURL=webpack-internal:///(ssr)/./node_modules/available-typed-arrays/index.js\n");

/***/ })

};
;