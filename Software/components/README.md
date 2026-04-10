This folder holds the custom components.

# Changes to the component
When changes are made to the component the version in the ``idf_component.yml`` should be increased for more information see [versioning](https://docs.espressif.com/projects/idf-component-manager/en/latest/reference/versioning.html). 

# Add a component as dependency to a component
1. Change the directory to the component where you want to add the dependency
2. Use the ``idf.py add-dependency`` command as described [here](https://docs.espressif.com/projects/idf-component-manager/en/latest/use/how_to_add_dependency.html) to add the component as a dependency for the current component
3. After reconfiguring the project the dependency should get a entry in the ``managed_components/`` folder
