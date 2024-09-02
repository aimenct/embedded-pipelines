Data Types
##########

BaseTypes
---------

BaseType enumeration represents the fundamental data types used in the framework.

.. list-table:: BaseType Enumeration and Corresponding C++ Data Types
   :header-rows: 1
   :widths: 15 25 20

   * - **BaseType**
     - **C++ Data Type**
     - **Size (bytes)**
   * - EP_8C
     - char_t
     - 1
   * - EP_8U
     - uint8_t
     - 1
   * - EP_8S
     - int8_t
     - 1
   * - EP_16U
     - uint16_t
     - 2
   * - EP_16S
     - int16_t
     - 2
   * - EP_32U
     - uint32_t
     - 4
   * - EP_32S
     - int32_t
     - 4
   * - EP_64S
     - int64_t
     - 8
   * - EP_64U
     - uint64_t
     - 8
   * - EP_32F
     - float
     - 4
   * - EP_64F
     - double
     - 8
   * - EP_BOOL
     - bool
     - Platform dependent
   * - EP_STRING
     - std::string
     - Variable

Object Types
------------

`ObjectType` is defined to categorize different kinds of objects used within the framework. Each object type is associated with a specific integer value that identifies its category. Developers can define custom object types in their applications beyond the predefined types.

Objects represent complex data structures as node trees. The default object is a simple nodetree

.. list-table:: Overview of Object Types
   :header-rows: 1
   :widths: 15 70

   * - **Object Type**
     - **Description**
   * - EP_OBJ (0)
     - Generic category for objects that do not fit into more specific ones.
   * - EP_IMAGE_OBJ (1)
     - Image object that handle image data.

Represents a general object type. This is a default or base object type serving as a generic category for objects that do not fit into more specific categories.
distinguishing them from other types of data objects within the framework.

Image Object
^^^^^^^^^^^^
- **ImageObject**: Is use to handle image data. It implements a specific nodetree to handle image data with additional methods for easy access and management.
  The representation of an image within the framework follow the next node tree structure.

  .. code-block:: text

   ObjectNode <Image, name>
		  hasChild   DataNode <width>
		  hasChild   DataNode <height>
		  hasChild   DataNode <channels>
		  hasChild   DataNode <PixelFormat>
		  hasChild   DataNode <data>

.. https://reference.opcfoundation.org/Core/Part3/v104/docs/A.4.3
