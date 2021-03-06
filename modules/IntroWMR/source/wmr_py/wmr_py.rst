Counting words with WebMapReduce (WMR): adding efficiency
=========================================================

Introduction
------------

For this activity, you need to have read the accompanying
background reading in the first section entitled
*Using Parallelism to Analyze Very Large Files: Google's MapReduce Paradigm*.
Also, you should have gone through the previous section
where you learned to use WMR on a simple word-counting example.

WMR has several languages as options.  In this case we will
demonstrate an improvement that can be made easily if you are using
the Python language, because it has dictionaries, or hash maps,
as a built-in data type.


An improved word-count example
-----------------------------------------------------------

As in the previous section, we will start with a small example
as an illustration.  In this case we will describe an improvement in 
which mappers do a bit more work by keeping counts of words it has
encountered and then emitting each word and its total count to
the system for the reducer processes to handle.  In a map-reduce system,
it turns out to be useful to let the mappers do a fair amount of work,
such as processing a whole book, since this is a reasonable task
for a single process.

As an example, consider the problem of counting how frequently each
word appears in a collection of data. For example, if the input
data in a file is:

::

    One fish, Two fish,
    Red Fish, Blue fish.

then the output should be:

::

    Blue 1
    One 1
    Red 1
    Two 1
    Fish, 1
    fish, 2
    fish. 1

As this above output indicates, we did not make any attempt to trim
punctuation characters.  If we were to
make sure that we stripped punctuation and used lowercase characters
for each word encountered, we would get this:

::

    blue 1
    one 1
    red 1
    two 1
    fish 4
    

What follows is a plan for the mapper and reducer functions. You
should compare and note the similarity between these and a
sequential function for completing this same task on a single input
file.


Map-reduce plan
^^^^^^^^^^^^^^^

In WMR, mapper functions work simultaneously on lines of input from
files, where a line ends with a newline charater. The mapper will
produce one key-value pair (*w*, *count*) foreach word encountered
in the input line that it is working on.

Thus, on the above input, two mappers working together on each line,
after removing punctuation from the end of words and converting the
words to lowercase
would emit the following combined output:

::

    one 1
    fish 2
    two 1

    red 1
    fish 2
    blue 1
    

The reducers will compute the sum of all the *count* values for a
given word *w*, then produce the key-value pair (*w*, *sum*).


The mapper function
-------------------

The pseudocode for the mapper looks like this::

  # key is a single line from a file.
  # value is empty in this case, since this is the first mapper function
  # we are applying.
  #
  function mapper(key, value)
    1) Create a dictionary or hash table called counts
    (the keys in counts will be words found and the values will be counts of each word)

    2) Take the key argument to this function, which is the line of text, 
       and split it on whitespace
    
    3) For every word resulting from the split key line:
        strip puntuation from the word
        convert the word to lowercase
        if the word is already a key in the counts dictionary, then
          increment the value in the counts dictioanry by one
        else
          add the key, value pair of (word, 1) to the counts dictionary

    4) For every word 'key' now in the dictionary
        'emit' the (word, count) to the system for the reducers to handle

Here is a Python3 mapper function for accomplishing this task using
the WMR system. We also add the feature of stripping away
puncuation characters from the input.


.. literalinclude::  wc_comb_mapper.py
    :linenos:
    :language: python


This code is available :download:`for download as wc_comb_mapper.py <wc_comb_mapper.py>`.
You can use this file later when you wish to use it as your mapper in WMR.

Let's examine this code carefully. In line 1 we import the Python
``string`` package so that we can use its method for returning
punctuation characters, found in line 7. Line 3 shows how all
mapper functions in WMR should be defined, with two parameters
called `key` and `value`. Each of these parameters is a *String*
data type. In the case of our first mapper functions reading each
line of the file, the whole line is passed into the key in the
map-reduce system underlying WMR, and the value is empty. (See
additional notes section below for more details you will need when
trying other examples.)

In line 4, we create a Python dictionary called `counts` to hold
each distinct word and the number of time it appears. In the small
input example we describe here, this will not have many entries.
When we next read files where a whole book may be contained in one
line of data, the dictionary called counts will contain many
words.

Line 5 is where we take the input line, which was in the `key`, and
break it into words. Then the loop in lines 6-11 goes word by word
and strips punctuation and increments the count of that word.

The loop in lines 13 and 14 is how we send the data off to the
reducers. The WMR system for Python3 defines a class ``Wmr`` that
includes a class method ``emit()`` for producing key-value pairs to
be forwarded (via shuffling) to a reducer. ``Wmr.emit()`` requires
two string arguments, so both `foundword` and `counts[foundword]`
are being interpreted as strings in line 14.


The reducer function
--------------------

Pseudocode for a reducer for this problem is exactly the same as
for the previous section and looks like this::

  # key is a single word, values is a 'container' of counts that were
  # gathered by the system from every mapper
  #
  function reducer(key, values)
    
    set a sum accumulator to zero

    for each count in values
      accumulate the sum by adding count to it

    'emit' the (key, sum) pair


A reducer function for solving the word-count problem in Python is


.. literalinclude::  wcreducer.py
    :linenos:
    :language: python


This code is available :download:`for download as wcreducer.py <wcreducer.py>`.
You can use this file later when you wish to use it as your reducer in WMR.


The function ``reducer()`` is called once for each distinct key
that appears among the key-value pairs emitted by the mapper, and
that call processes all of the key-value pairs that use that key.
On line 1, the two parameters that are arguments of ``reducer()``
are that one distinct ``key`` and a Python3 *iterator* (similar to a
list, but not quite) called ``values``, which provides access to
all the values for that key. Iterators in Python3 are designed for
``for`` loops- note in line 3 that we can simply ask for each value
one at a time from the set of values held by the iterator.

*Rationale:* WMR ``reducer()`` functions use iterators instead of
lists because the number of values may be very large in the case of
large data. For example, there would be billions of occurrences of
the word "the" if our data consisted of all pages on the web. When
there are a lot of key-value pairs, it is more efficient to
dispense pairs one at a time through an iterator than to create a
gigantic complete list and hold that list in main memory; also, an
enormous list may overfill main memory.

The ``reducer()`` function adds up all the counts that appear in
key-value pairs for the ``key`` that appears as ``reducer()``'s
first argument (recall these come from separate mappers). Each
count provided by the iterator ``values`` is a string, so in line 4
we must first convert it to an integer before adding it to the
running total ``sum``.

The method ``Wmr.emit()`` is used to produce key-value pairs as
output from the mapper. This time, only one pair is emitted,
consisting of the word being counted and ``sum``, which holds the
number of times that word appeared in *all* of the original data.

Running the example code on WebMapReduce
----------------------------------------

If you have not registered a WMR account or tried the example in the previous section,
do that first so that you are used to setting up a job in WMR.  You can runn
the above mapper and reducer code files on the simple example above in 'Test'
mode on WMR to ensure that they work.



Next Steps
----------


#. In WMR, you can choose to use your own input data files. Try
   choosing to 'browse' for a file, and using ``mobydick.txt`` as file
   input.

#. There are a large number of files of books from Project
   Gutenberg available on the Hadoop system underlying WebMapReduce.
   First start by trying this book as an input file by choosing
   'Cluster Path' as Input in WMR and typing one of these into the
   text box:

   | /shared/gutenberg/WarAndPeace.txt
   | /shared/gutenberg/CompleteShakespeare.txt
   | /shared/gutenberg/AnnaKarenina.txt

   These books have many lines of text with 'newline' chacters at the
   end of each line. Each of you mapper functions works on one line.
   Try one of these.

#. Next, you should try a collection of many books, each of which
   has no newline characters in them. In this case, each mapper 'task'
   in WMR;s underlying Hadoop system will work on one whole book (your dictionary of words per
   mapper will be for the whole book, and the mappers will be running
   on many books at one time). *This new version should run faster in principle
   on the system by letting mappers do a bit of work and pass lass data
   to the awaiting reducers.*  You might want to see if you can see a
   'wall clock' time difference bewteen this version and the code
   described in the previous section.  Keep in mind, however, that
   the time to run depends on how many other users are also using the system.

   In the Hadoop file system inside WMR we
   have these datasets available for this:

       ===================================    ================
       'Cluster path' to enter in WMR         Number of books
       ===================================    ================
       /shared/gutenberg/all\_nonl/group10    2018
       /shared/gutenberg/all\_nonl/group11    294
       /shared/gutenberg/all\_nonl/group6     830
       /shared/gutenberg/all\_nonl/group8     541
       ===================================    ================

   While using many books, it will be useful for you to experiment
   with the different datasets so that you can get a sense for how
   much a system like Hadoop can process.

   To do this, it will also be useful for you to save your
   configuration so that you can use it again with a different number
   of reducer tasks. Once you have entered your mapper and reducer
   code, picked the Python3 language, and given your job a descriptive
   name, choose the `'Save'` button at the bottom of the WMR panel.
   This will now be a `'Saved Configuration'` that you can retrieve
   using the link on the left in the WMR page.

   Try using the smallest set first (group11). Do not enter anything
   in the map tasks box- notice that the system will choose the same
   number of mappers as the number of books (this will show up once
   you submit the job). Also do not enter anything for the number of
   reduce tasks. With that many books, when the job completes you will
   see there are many pages of output, and some interesting 'words'.
   For the 294 books in group11, note how you obtain several pages of
   results. You will also notice that the stripping of punctuation
   isn't perfect. If you wish to try improving this you could, but it
   is not necessary.


Additional Notes
----------------

These notes are repeated from the previous section.

It is possible that input data files to mappers may be treated
differently than as described in the above example. For example, a
mapper function might be used as a second pass by operating on the
reducer results from a previous map-reduce cycle. Or the data may
be formatted differently than a text file from a novel or poem.
These notes pertain to those cases.

In WMR, each line of data is treated as a key-value pair of
strings, where the key consists of all characters before the first
``TAB`` character in that line, and the value consists of all
characters after that first ``TAB`` character. Each call of
``mapper()`` operates on one line and that function's two arguments
are the key and value from that line.

If there are multiple ``TAB`` characters in a line, all ``TAB``
characters after the first remain as part of the ``value`` argument
of ``mapper()``.

If there are *no* ``TAB`` characters in a data line (as is the case
with all of our fish lines above), an empty string is created for
the value and the entire line's content is treated as the key. This
is why the key is split in the mapper shown above.


