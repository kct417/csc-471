The scene is an adaptation of a pokemon forest where pokemon wander around. There is a pokemon trainer waving at the camera. The pokemon are multimesh shapes that are scattered around the camera.

Colors were chosen based on the color that the pokemon would have in the show (the main color of the pokemon). Pressing m will convert the Eevee color to that of a shiny Eevee.

Demonstration of handling a mesh with no normals included in default execution, the mesh with no normals is the heirarchical model from icoNoNormals, located in the science at resources.

Each pokemon and the hierarchical model contains a shadow that should be calculated based on a flattening of the y axis. When pressing x the hierarchical model should animate and the shadow should be computed based on that animated position.

The ground texture was converted to one of grass to match the pokemon forest theme. The texture used was the grass texture found in the resources folder.

The shader now also takes in the light color which is passed in as a uniform which can be changed later. It is only white in this instance. There is also a shadow shader that is used to implement the shadows in the scene. The simple vert shader is reused in this case.

Lighting will be translated slightly left and right when pressing q and e.
