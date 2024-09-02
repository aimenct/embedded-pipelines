Tutorials
=========

Getting Started
---------------
In this tutorial you will learn:
- how to create a very simple pipeline with one data source filter.
- how to retreive data from a queue in the main section using a queue readers.

tutorial_01.cpp
  
.. code-block:: cpp
	// Declare the Message Pointers
	EdgeMessage *src_msg;

Building with cmake
	
-- code-block:: cmake
	

Connecting Filters
------------------
Create a pipeline with four instances. Assignment of threads to filters' job.
Play with queue settings parameters (queue type, blocking calls, etc.)

tutorial_02.cpp

.. code-block:: cpp		


Creating Custom Filters
-----------------------
A guide to developing your own Filters for specific tasks.

tutorial_03.cpp

.. code-block:: cpp

	// Declare the Message Pointers
	EdgeMessage *src_msg;
	EdgeMessage *sink_msg;

	// Begin the infinite loop of the Filter thread
	while( True ){
		// Retrive pointers from the Source and Sink Queues
		int err_r = src_queue.startRead( src_msg );
		int err_w = sink_queue.startWrite( sink_msg );
		
		// Retrieve nodes to build the object helpers
		// Efficiency, just filling ptr
		Node* item_node_0 = src_msg.item( 0 );
		Node* item_node_1 = src_msg.item( 1 );
		
		Node* item_node_2 = sink_msg.item( 2 );
		Node* item_node_3 = sink_msg.item( 3 );
		
		// Build object helpers
		EdgeImage src_img( item_node_0 );
		EdgeImage sink_img( item_node_2 );
		
		// You can use other library objects to perform the processing more easily
		cv::Mat mat_src_img( src_img.height(), src_img.width(), src_img.type(), 
		src_img.value(), step=AUTO_STEP) ); 
		
		cv::Mat mat_sink_img( sink_img.height(), sink_img.width(), sink_img.type(), 
		sink_img.value(), step=AUTO_STEP) );
		
		// Apply the processing of the filter
		cv::blur( mat_src_img, mat_sink_img, Size(3,3) );
		
		int err_r = src_queue.endRead();
		int err_w = sink_queue.endWrite();
	}



OPC-UA example
--------------
Tips and tricks for optimizing performance and integrating with other systems.
For this tutorial you will need to activate OPC-UA module when compiling the library

tutorial_04.cpp


Camera example
------------------

camera_pipeline.cpp


File Sink and File Source
-------------------------

