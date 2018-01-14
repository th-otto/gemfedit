Compiling FreeType 2 with PureC compiler
========================================

[See below for a German version.]

To compile FreeType 2 as a library the following changes must be applied:

- You have to change the INCLUDE directory in PureC's compiler options
  to contain both the `INCLUDE' and `freetype2\include' directory.
  Example:

    INCLUDE;E:\freetype2\include




Compilieren von FreeType 2 mit PureC
====================================

Um FreeType 2 als eine Bibliothek (library) zu compilieren, muss folgendes
ge„ndert werden:

- In den Compiler-Optionen von PureC muss das INCLUDE directory auf INCLUDE
  und freetype2\include verweisen. Z.B.:

    INCLUDE;E:\freetype2\include

--- end of README.TXT ---
