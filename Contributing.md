Here's an incomplete list of the ways you can help make Cusp better.

# Bug Reports #

# Feedback #

# Feature Requests #

# Documentation #

We strive to make Cusp's [Documentation](Documentation.md) as accurate and informative as possible.  However, if you find the documentation incomplete or misleading in a certain area, please let us know.  Either report an [issue](http://code.google.com/p/cusp-library/issues/list) or send a message to [cusp-users](http://groups.google.com/group/cusp-users).

# Source Code #

Last, but not least, you can contribute source code to Cusp.  Some ideas you might consider are listed in our [Roadmap](Roadmap.md), but feel free to suggest others.  In either case, please let us know your intentions by posting to [cusp-users](http://groups.google.com/group/cusp-users) so we can determine a plan of action.

## Developing with Mercurial ##

Begin by [creating a personal "clone" of the Cusp repository](http://code.google.com/p/cusp-library/source/createClone).  This creates a copy of the mainstream Cusp repository but hosts it under a different name. The clone will serve as an external repository to host changes before they are ready to be integrated back into the mainstream Cusp repository.

You can clone your clone to your development machine like so:

```
$ hg clone https://developername-clonename.googlecode.com/hg cusp-clonename
```

Develop in your local clone:

```
$ cd cusp-clonename
<<<hack hack hack>>>
```

Commit changes locally, and push them back to your personal repository:

```
$ hg commit -u <developername>
$ hg push
```

Periodically integrate changes from the mainstream Cusp repository to ensure your clone doesn't fall too far out of sync:

```
$ hg pull https://cusp-library.googlecode.com/hg
$ hg up
$ hg push
```

When you are ready to propose your change, request a code review by sending a message to cusp-users.

Finally, a developer with commit access to the mainstream Cusp repository will commit your changes:

```
$ hg clone https://cusp-library.googlecode.com/hg cusp-merge-contribution
$ cd cusp-merge-contribution
$ hg pull https://developername-clonename.googlecode.com/hg
$ hg up
$ hg commit -u <committername>
$ hg push
```