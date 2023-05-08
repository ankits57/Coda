#include "Environment.h"
#include "../../Error/Error.h"
namespace Coda {
	namespace Runtime {
		Environment::Environment()
		{
			parent = nullptr;
		}
		Environment::Environment(Environment* parentEnvironment)
		{
			this->parent = parentEnvironment;
		}

		Environment Environment::root()
		{
			Environment env = Environment();

			env.declareOrAssignVariable("none", Value(Type::NONE, "none", Error::Position(), Error::Position()), true);
			env.declareOrAssignVariable("undefined", Value(Type::UNDEFINED, "undefined", Error::Position(), Error::Position()), true);
			env.declareOrAssignVariable("true", Value(Type::BOOL, "1", Error::Position(), Error::Position()), true);
			env.declareOrAssignVariable("false", Value(Type::BOOL, "0", Error::Position(), Error::Position()), true);

			return env;
		}


		Value Environment::declareOrAssignVariable(std::string name, Value value, bool isConstant)
		{

			auto it = symbols.find(name);
			if (it != symbols.end()) { // aready exists, assign
				if (isConstant)
				{
					Error::Runtime::raise("Reassignment of constant is not allowed, at ", value.endPosition);
					return value;
				}
				else if (constants.find(name) != constants.end()) {	// let declaration but constant exists.
					Error::Runtime::raise("Reassignment of constant is not allowed, at ", value.endPosition);
					return value;
				}
				symbols.insert_or_assign(name, value);
			}
			else { // doesn't exist, create
				Environment* env = resolve(name);

				if (!Error::Manager::isSafe()) return Value();

				if (env != nullptr)
				{
					env->symbols.insert({ name, value });

					if (isConstant)
						env->constants.insert(name);
				}
				else {
					symbols.insert({ name, value });
					if (isConstant)
						constants.insert(name);
				}
			}
			return value;
		}
		Value Environment::lookupSymbol(std::string name)
		{
			Environment* env = resolve(name);
			if (env != nullptr)
				return env->symbols[name];
			else
				Error::Runtime::raise("Symbol '" + name + "' does not exist");
			return Value();

		}

		Environment* Environment::resolve(std::string name)
		{
			auto it = symbols.find(name);
			if (it != symbols.end()) {
				return this;
			}

			if (parent == nullptr) {
				return nullptr;
			}

			return parent->resolve(name);
		}
	}
}
