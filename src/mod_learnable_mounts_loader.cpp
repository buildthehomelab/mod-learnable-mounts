/*
 * mod-learnable-mounts loader.
 *
 * The playerbots fork auto-globs every module's sources into one lib and looks up a loader
 * symbol derived from the folder name: for folder "mod-learnable-mounts" that symbol is exactly
 * "Addmod_learnable_mountsScripts". It must exist and call our real registration function.
 *
 * Released under GNU GPL v2 or (at your option) any later version.
 */

void AddLearnableMountsScripts();

void Addmod_learnable_mountsScripts()
{
    AddLearnableMountsScripts();
}
