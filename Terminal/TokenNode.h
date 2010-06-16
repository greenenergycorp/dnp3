#ifndef __TOKEN_NODE_H_
#define __TOKEN_NODE_H_

#include <map>
#include <vector>
#include <string>
#include <assert.h>

namespace apl
{
	template <class T>
	class TokenNode
	{
		typedef std::map<std::string, TokenNode<T>*> TokenMap;

		public:

			TokenNode(T aValue)
				:
				mHasDefault(true),
				mValue(aValue)
			{

			}

			TokenNode()
				:
				mHasDefault(false)
			{

			}

			~TokenNode()
			{
				typename TokenMap::iterator i = mMap.begin();
				for(; i != mMap.end(); i++) delete (i->second);
			}

			T* GetValue()
			{
				return &mValue;
			}


			// returns the correct handler for the tokens
			// and modifies the Tokens vector appropriately for the call
			TokenNode* FindNode(std::vector<std::string>& arTokens)
			{
				if(arTokens.size() == 0) return this;

				typename TokenMap::iterator i = mMap.find(arTokens[0]);

				if(i == mMap.end()) return this; // this node with arguments

				//strip and argument and call the next node
				arTokens.erase(arTokens.begin());
				return (i->second)->FindNode(arTokens);
			}


			TokenNode* AddToken(const std::string& arToken, T aValue)
			{
				assert(mMap.find(arToken) == mMap.end());
				TokenNode<T>* pToken = new TokenNode<T>(aValue);
				mMap[arToken] = pToken;
				return pToken;
			}


			void SetValue(T aValue)
			{
				mHasDefault = true;
				mValue = aValue;
			}

			void GetSubNodeNames(std::vector<std::string>& arNames)
			{
				typename TokenMap::iterator i = mMap.begin();
				for(; i != mMap.end(); i++) arNames.push_back(i->first);
			}


		private:

			bool mHasDefault;

			T mValue;

			TokenMap mMap;


	};
}

#endif
