#include "../unity_observer.h"

// change depending on invokation method
#define SIG_RETURN_VALUE_TYPE(name) Types::BoxedValue<name>*
#define SIG_RETURN_PRIM_TYPE(name) name
#define SIG_PARAM_VALUE_TYPE(name) Types::BoxedValue<name>*

struct Vector3 {
  float x;
  float y;
  float z;

  static inline Runtime::Class* StaticClass() {
	  static auto clazz = Runtime::Domain::GetRootDomain()->GetAssembly("UnityEngine.CoreModule")->GetClass("UnityEngine", "Vector3");
	  return clazz;
  }

  static inline SIG_RETURN_PRIM_TYPE(float) Distance(SIG_PARAM_VALUE_TYPE(Vector3) a, SIG_PARAM_VALUE_TYPE(Vector3) b) {
	  static auto method = StaticClass()->GetMethod("Distance");
	  return method->InvokeFast<SIG_RETURN_PRIM_TYPE(float)>(nullptr, a, b);
  }
};
