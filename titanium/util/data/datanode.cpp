#include "datanode.hpp"

#include "titanium/util/assert.hpp"

namespace utils::data
{
    const char * DataNode::ValueTypeToString(const eValueType eType)
    {
        /*static const stl::Map<eValueType, const char *> mappszTypeToString {
            { eValueType::BOOL, "bool" },
            { eValueType::INT, "int"},
            { eValueType::FLOAT, "float"},
            { eValueType::STRING, "string"},
            { eValueType::CHILD, "DataNode children"}
        };

        return mappszTypeToString.at(eType);*/

        #warning "Add map implementation"

        return nullptr;
    }

    DataNode::eValueType DataNode::GetValueType() const
    {
        return m_eValueType;
    }

    bool DataNode::HasValue() const
    {
        return m_eValueType != eValueType::__MAX;
    }

    const char * DataNode::GetString() const
    {
        // TODO: should we allow conversion to string from non-string values here? could make a ToString() func for this
        assert::Release(m_eValueType == eValueType::STRING, "Tried to get string from DataNode, while DataNode has type %s", ValueTypeToString(m_eValueType));
        return mu_pszValue;
    }

    i32 DataNode::GetInt() const 
    {
        assert::Release(m_eValueType == eValueType::INT, "Tried to get int from DataNode, while DataNode has type %s", ValueTypeToString(m_eValueType));
        return mu_nValue;
    }

    f32 DataNode::GetFloat() const
    {
        assert::Release(m_eValueType == eValueType::FLOAT, "Tried to get float from DataNode, while DataNode has type %s", ValueTypeToString(m_eValueType));
        return mu_flValue;
    }

    bool DataNode::GetBool() const
    {
        // TODO: should we allow casting to bool from non-bool values here? could make a ToBool() func for this
        assert::Release(m_eValueType == eValueType::BOOL);
        return mu_nValue != 0;
    }

    DataNode * DataNode::GetChild(const char *const pszName) const
    {
        assert::Release(m_eValueType == eValueType::CHILD);
        return nullptr; //&mu_pmappdnChildren->at(pszName);
    }
}
