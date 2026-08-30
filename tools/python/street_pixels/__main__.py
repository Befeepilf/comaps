import argparse
import sys

from street_pixels.map_pipeline import main as map_pipeline_main
from street_pixels.prepare_spa_debug_root import main as prepare_main
from street_pixels.serve_spa_publish_tree import main as serve_main


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(
        description="Street Pixels maintainer tools",
        usage="""python3 -m street_pixels <command> [<args>]
commands:
    map_pipeline             Build-host generate: mapgen → pix → rings → spa → assemble (SP-100)
    prepare_spa_debug_root   Fetch CDN countries + assemble spa publish root (debug, not production)
    serve_spa_publish_tree   Serve SP-050 publish tree (SP-051)
""",
    )
    parser.add_argument("command", help="Subcommand to run")
    args = parser.parse_args(argv[:1])
    if args.command == "map_pipeline":
        return map_pipeline_main(argv[1:])
    if args.command == "serve_spa_publish_tree":
        return serve_main(argv[1:])
    if args.command == "prepare_spa_debug_root":
        return prepare_main(argv[1:])
    parser.error("Unrecognized command {!r}".format(args.command))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
