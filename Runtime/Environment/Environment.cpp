#include "Environment.h"

#include "../NativeFunctions/NativeFunction.h"


namespace Coda {
	namespace Runtime {

		Environment::Environment()
		{
			mParent = nullptr;
		}
		Environment::Environment(Environment* parentEnvironment)
		{
			this->mParent = parentEnvironment;
		}

		Environment Environment::root()
		{
			Environment env = Environment();

			env.declareOrAssignVariable("none", std::make_shared<Value>(Type::NONE, "none"), true);
			env.declareOrAssignVariable("undefined", std::make_shared<Value>(Type::UNDEFINED, "undefined"), true);
			env.declareOrAssignVariable("true", std::make_shared<Value>(Type::BOOL, "1"), true);
			env.declareOrAssignVariable("false", std::make_shared<Value>(Type::BOOL, "0"), true);

			// i/o
			env.declareNativeFunction("print", Native::print);
			env.declareNativeFunction("println", Native::println);
			env.declareNativeFunction("input", Native::input);
			env.declareNativeFunction("sleep", Native::sleep);

			// quit
			env.declareNativeFunction("quit", Native::quit);
			env.declareNativeFunction("exit", Native::quit);

			// conversion
			env.declareNativeFunction("parseInt", Native::parseInt);
			env.declareNativeFunction("parseFloat", Native::parseFloat);
			env.declareNativeFunction("parseDouble", Native::parseDouble);
			env.declareNativeFunction("parseByte", Native::parseByte);
			env.declareNativeFunction("parseBool", Native::parseBool);

			return env;
		}

		IValuePtr Environment::declareFunctionParameter(const std::string name, const IValuePtr& value) {
			auto symbolIt = mSymbols.find(name);
			if (symbolIt != mSymbols.end()) { // Prameters cannot exist with same name
				Error::Runtime::raise("Parameter with same name already exists, ",Interpreter::callStack,value->getStartPosition(), value->getEndPosition());
				return nullptr;
			}
			else {
				mSymbols.emplace(name, value);
				mConstants.insert(name);
				return value;
			}
		}

		IValuePtr Environment::declareOrAssignVariable(const std::string& name, const IValuePtr& value, bool isConstant)
		{
			auto symbolIt = mSymbols.find(name);
			if (symbolIt != mSymbols.end()) {
				// Variable already exists, assign to it
				if (isConstant) {
					Error::Runtime::raise("Reassignment of constant is not allowed, ", Interpreter::callStack, value->getStartPosition(), value->getEndPosition());
					return nullptr;
				}
				else if (mConstants.find(name) != mConstants.end()) {
					// Trying to assign to a constant variable
					Error::Runtime::raise("Assignment to constant variable is not allowed, ", Interpreter::callStack, value->getStartPosition(), value->getEndPosition());
					return nullptr;
				}
				symbolIt->second = std::dynamic_pointer_cast<Value>(value);
			}
			else {
				// Variable does not exist, check in parent environment
				if (mParent != nullptr) {
					Environment* env = resolveWithParentAndScope(name);
					if (env != nullptr)
						return mParent->declareOrAssignVariable(name, value, isConstant);
				}

				// Variable does not exist in parent environment, declare it
				if (isConstant) {
					mConstants.insert(name);
				}
				mSymbols.emplace(name, value);

			}
			return value;
		}

		IValuePtr Environment::declareOrAssignVariable(const Frontend::Node& name, const IValuePtr& value, bool isConstant)
		{
			if (name.type != Frontend::NodeType::MEMBER_EXPRESSION) {
				Error::Runtime::raise("Invalid variable name");
				IF_ERROR_RETURN_VALUE_PTR;
			}

			const Frontend::Node* current = &name;
			std::vector<std::string> identifierChain;

			// Navigate inwards through MemberExpression nodes
			while (current->type == Frontend::NodeType::MEMBER_EXPRESSION) {
				const Frontend::Node& left = *current->left;
				const Frontend::Node& right = *current->right;

				identifierChain.push_back(right.value);
				current = &left;
			}

			std::reverse(identifierChain.begin(), identifierChain.end());
			ValuePtr variableValue;
			variableValue = std::dynamic_pointer_cast<Value>(mSymbols[current->value]);

			for (int i = 0; i < identifierChain.size() - 1; i++) {
				variableValue = std::dynamic_pointer_cast<Value>(variableValue->properties[identifierChain[i]]);
			}

			variableValue->properties[identifierChain.back()] = std::make_shared<Value>(value);
			return value;
		}

		IValuePtr Environment::declareNativeFunction(const std::string& name,IEnvironment::Function function)
		{
			declareOrAssignVariable(name, std::make_shared<Value>(Type::NATIVE_FUNCTION, name), true);
			mFunctions.emplace(name, function);
			return nullptr;
		}

		IValuePtr Environment::declareUserDefinedFunction(const std::string& name, const INode& astNode)
		{
			ValuePtr function = std::make_shared<Value>(Type::FUNCTION, astNode.getValue());
			this->declareOrAssignVariable(name, function, false);
			return function;
		}

		IValuePtr Environment::callFunction(const std::string& name, IValuePtr args, IEnvironment& env) {
			auto it = mFunctions.find(name);
			if (it != mFunctions.end()) {
				return it->second(args, &env);
			}
			else if (mParent != nullptr) {
				return mParent->callFunction(name, args, env);
			}
			else {
				Error::Runtime::raise("Function '" + name + "' does not exist in the scope.");
				return nullptr;
			}
		}

		IValuePtr Environment::lookupSymbol(std::string name)
		{
			Environment* env = resolveWithParentAndScope(name);
			if (env != nullptr)
				return env->mSymbols[name];
			else
				Error::Runtime::raise("Symbol '" + name + "' does not exist");
			return nullptr;
		}

		Environment* Environment::resolveWithParentAndScope(std::string name)
		{
			auto it = mSymbols.find(name);
			if (it != mSymbols.end()) {
				return this;
			}

			for (auto& it : mScopes) {
				Environment* scope = it.second->resolveWithScope(name);
				if (scope != nullptr)
					return scope;
			}

			if (mParent == nullptr) {
				return nullptr;
			}

			return mParent->resolveWithParentAndScope(name);
		}

		Environment* Environment::resolveWithScope(std::string name)
		{
			auto it = mSymbols.find(name);
			if (it != mSymbols.end()) {
				return this;
			}

			for (auto& it : mScopes) {
				Environment* scope = it.second->resolveWithParentAndScope(name);
				if (scope != nullptr)
					return scope;
			}

			return nullptr;
		}

		Environment::UserDefinedFunction* Environment::getFunction(const std::string& name) {
			auto it = std::find_if(mUserDefinedFunctions.begin(), mUserDefinedFunctions.end(),
				[&](auto func) {
					return std::get<0>(func) == name;
				});

			if (it != mUserDefinedFunctions.end()) {
				return &(*it);
			}

			for (auto& it : mScopes) {
				Environment::UserDefinedFunction* func = it.second->getFunctionInScope(name);
				if (func != nullptr)
					return func;
			}

			if (this->mParent) {
				auto func = this->mParent->getFunction(name);
				return func;
			}
			Error::Runtime::raise("Function '" + name + "' does not exist");
			return nullptr;
		}

		Environment::UserDefinedFunction* Environment::getFunctionInScope(const std::string& name)
		{
			auto it = std::find_if(mUserDefinedFunctions.begin(), mUserDefinedFunctions.end(),
				[&](auto func) {
					return std::get<0>(func) == name;
				});

			if (it != mUserDefinedFunctions.end()) {
				return &(*it);
			}

			for (auto& it : mScopes) {
				Environment::UserDefinedFunction* func = it.second->getFunctionInScope(name);
				if (func != nullptr)
					return func;
			}
			return nullptr;
		}

		ValuePtr Environment::addFunction(const std::string& name, const Frontend::Node& astNode, Environment& env)
		{
			mUserDefinedFunctions.push_back(UserDefinedFunction(astNode.value, &env, astNode));
			return std::dynamic_pointer_cast<Value>(env.declareUserDefinedFunction(astNode.value, astNode));
		}

		void Environment::remove(const std::string& name)
		{
			//find in functions
			auto itF = mFunctions.find(name);
			if (itF != mFunctions.end()) {
				Error::Runtime::raise("Cannot remove native function");
				return;
			}

			// find the symbol
			auto itS = mSymbols.find(name);
			if (itS != mSymbols.end()) {
				mSymbols.erase(itS);
				return;
			}

			// find the constant
			auto itC = mConstants.find(name);
			if (itC != mConstants.end()) {
				Error::Runtime::raise("Cannot remove constant");
				return;
			}

			// find the user defined function
			auto itU = std::find_if(mUserDefinedFunctions.begin(), mUserDefinedFunctions.end(),
				[&](auto func) {
					return std::get<0>(func) == name;
				});

			if (itU != mUserDefinedFunctions.end()) {
				mUserDefinedFunctions.erase(itU);
				return;
			}

			Error::Runtime::raise("Symbol '" + name + "' does not exist");
		}
		void Environment::addScope(const std::string& name, std::shared_ptr<Environment> env)
		{
			if (mScopes.find(name) != mScopes.end()) {
				Error::Runtime::raise("Scope '" + name + "' already exists, cannot have scopes with same name.");
				return;
			}
			mScopes[name] = env;
			mSymbols[name] = std::make_shared<Value>(Type::SCOPE, name);
		}

		std::shared_ptr<Environment> Environment::getScopeForFunctionInScope(const std::string& name)
		{
			for (auto it : mScopes) {
				if (it.second->lookupSymbol(name) != nullptr) {
					std::shared_ptr<Environment> env = it.second;
					return env;
				}
			}

			return nullptr;
		}

		std::shared_ptr<Environment> Environment::getScope(const std::string& name) {
			std::shared_ptr<Environment> env;
			auto it = mScopes.find(name);
			if (it != mScopes.end()) {
				env = it->second;
				return env;
			}


			for(auto& scope : mScopes)
			{
				auto env = scope.second->getScopeInScope(name);
				if (env != nullptr)
					return env;
			}

			if (mParent != nullptr) {
				return mParent->getScope(name);
			}
			// If the scope doesn't exist in current or parent, return nullptr
			return nullptr;
		}

		std::shared_ptr<Environment> Environment::getScopeInScope(const std::string& name)
		{
			std::shared_ptr<Environment> env;
			auto it = mScopes.find(name);
			if (it != mScopes.end()) {
				env = it->second;
				return env;
			}

			for (auto& scope : mScopes)
			{
				auto env = scope.second->getScopeInScope(name);
				if (env != nullptr)
					return env;
			}

			// If the scope doesn't exist in current or parent, return nullptr
			return nullptr;
		}

	}
}