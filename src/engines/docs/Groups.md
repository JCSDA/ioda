Groups {#Groups}
----------------

\brief This page describes how to manipulate Groups. This includes accessing Variables and Metadata.

\see scratch::Group for the implementation.

Groups implement a directory-like structure. Files can contain Groups, Variables and Metadata.
Groups can also contain more Groups, Variables and Metadata.

All of these functions are defined in Group.h.


## Manipulating groups

Does a group exist as a child of the current object?
```
Group::exists(const std::string& name);
```

To create a group:
```
Group Group::create(const std::string& name);
```

To open a group:
```
Group Group::open(const std::string& name);
```

## Accessing objects inside of Groups

```
Has_Metadata meta;
Has_Variables vars;
```

