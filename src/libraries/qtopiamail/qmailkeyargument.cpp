/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

/*!
    \class QMailKeyArgument
    \inpublicgroup QtMessagingModule
    \inpublicgroup QtPimModule

    \preliminary
    \brief The QMailKeyArgument class template defines a class representing a single criterion 
    to be applied when filtering the QMailStore constent with a key object.
    \ingroup messaginglibrary

    A QMailKeyArgument\<PropertyType, ComparatorType\> is composed of a property indicator, 
    a comparison operator and a value or set of values to compare with.  The type of the 
    property indicator depends on the type that is to be filtered.
*/

/*!
    \typedef QMailKeyArgument::Property
    
    Defines the type used to represent the property that the criterion is applied to.

    A synomyn for the PropertyType template parameter with which the template is instantiated.
*/

/*!
    \typedef QMailKeyArgument::Comparator
    
    Defines the type used to represent the comparison operation that the criterion requires.

    A synomyn for the ComparatorType template parameter with which the template is instantiated; defaults to QMailDataComparator::Comparator.
*/

/*!
    \variable QMailKeyArgument::property
    
    Indicates the property of the filtered entity to be compared.
*/

/*!
    \variable QMailKeyArgument::op
    
    Indicates the comparison operation to be used when filtering entities.
*/

/*!
    \variable QMailKeyArgument::valueList
    
    Contains the values to be compared with when filtering entities.
*/

/*!
    \fn QMailKeyArgument::QMailKeyArgument()
    \internal
*/

/*!
    \class QMailKeyArgument::ValueList
    \inpublicgroup QtMessagingModule
    \inpublicgroup QtPimModule
    \ingroup messaginglibrary

    \brief The ValueList class provides a list of variant values that can be serialized to a stream, and compared.

    The ValueList class inherits from QVariantList.

    \sa QVariantList
*/

/*!
    \fn bool QMailKeyArgument::ValueList::operator==(const ValueList &other) const

    Returns true if this list and \a other contain equivalent values.
*/

/*!
    \fn void QMailKeyArgument::ValueList::serialize(Stream &stream) const

    Writes the contents of a ValueList to \a stream.
*/

/*!
    \fn void QMailKeyArgument::ValueList::deserialize(Stream &stream)

    Reads the contents of a ValueList from \a stream.
*/

/*!
    \fn QMailKeyArgument::QMailKeyArgument(Property p, Comparator c, const QVariant& v)

    Creates a criterion testing the property \a p against the value \a v, using the comparison operator \a c.
*/
    
/*!
    \fn QMailKeyArgument::QMailKeyArgument(const ListType& l, Property p, Comparator c)

    Creates a criterion testing the property \a p against the value list \a l, using the comparison operator \a c.
*/
    
/*!
    \fn bool QMailKeyArgument::operator==(const QMailKeyArgument<PropertyType, ComparatorType>& other) const
    \internal
*/

/*!
    \fn void QMailKeyArgument::serialize(Stream &stream) const

    Writes the contents of a QMailKeyArgument to \a stream.
*/

/*!
    \fn void QMailKeyArgument::deserialize(Stream &stream)

    Reads the contents of a QMailKeyArgument from \a stream.
*/

