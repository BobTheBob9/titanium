#pragma once

#include "titanium/util/numerics.hpp"
#include "titanium/util/data/map.hpp"

namespace utils::data
{
    /*
    
    Multiple-type data store, with support for named nested values

    */
    struct DataNode
    {
    private:
        // layered map
        struct ChildMap
        {
            
        };

        enum class eValueType
        {
            CHILD,
            STRING,
            INT,
            FLOAT,
            BOOL,

            __MAX
        } m_eValueType = eValueType::__MAX;

        union {
            // TODO: unsure whether we should be using an stl container here, reevaluate
            util::data::Map<const char *const, DataNode> * mu_pmappdnChildren;

            char * mu_pszValue;
            i32 mu_nValue;
            f32 mu_flValue;
        };

        // setter/getter funcs
    public:
        static const char * ValueTypeToString(const eValueType eType);

        void SetString(const char *const pszValue);
        void SetInt(const i32 iValue);
        void SetFloat(const f32 flValue);
        void SetBool(const bool bValue);

        /*
        
        Don't actually add any children, but set the current value type to children, and prepare for having children

        */
        void SetAsParent();
        /*
        
        Add a child to this node by name
        MEM: both name and object are copied during the call, so no need to allocate memory for either!

        */
        DataNode * AddChild(const char *const pszName, const DataNode dnValue);
        /*
        
        Remove a child from this node
        MEM: pszName is not stored during the call, so no need to allocate memory for it
             This does free any memory allocated for the child, however

        */
        void RemoveChild(const char *const pszName);
        /*
        
        Free all children of this node

        */
        void FreeChildren();

        eValueType GetValueType() const;
        bool HasValue() const;
        const char * GetString() const;
        // TODO: const char * ToString() const;
        i32 GetInt() const;
        f32 GetFloat() const;
        bool GetBool() const;
        // bool ToBool() const;

        DataNode * GetChild(const char *const pszName) const;

    };
};
