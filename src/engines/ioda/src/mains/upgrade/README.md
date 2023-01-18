The below description explains the history of the ioda data files:

* ioda v1:
    * original layout using a flat group hierarchy and using the '@' symbol to separate group and variable names.
* ioda v2: 
    * utilizes a single-level hierarchical group structure where associated variables appear with the same name in different groups.
    * Eg, air_temperature@ObsValue and air_temperature@ObsError become variables named air_temperature in two sub-groups named ObsValue and ObsError respectively.
* ioda v3:
    * same group and variable structure as ioda v2, but with the new OBS conventional variable and group names.
