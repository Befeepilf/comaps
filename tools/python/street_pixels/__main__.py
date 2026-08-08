import argparse
import sys

from street_pixels.serve_spa_publish_tree import main as serve_main


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(
        description="Street Pixels maintainer tools",
        usage="""python3 -m street_pixels <command> [<args>]
commands:
    serve_spa_publish_tree   Serve SP-050 publish tree (SP-051)
""",
    )
    parser.add_argument("command", help="Subcommand to run")
    args = parser.parse_args(argv[:1])
    if args.command == "serve_spa_publish_tree":
        return serve_main(argv[1:])
    parser.error("Unrecognized command {!r}".format(args.command))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
